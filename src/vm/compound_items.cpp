#include <algorithm>
#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/reference_counter.h>
#include <set>
#include <stdexcept>

namespace neo::vm
{
// ArrayItem implementation
ArrayItem::ArrayItem(const std::vector<std::shared_ptr<StackItem>>& value, ReferenceCounter* refCounter)
    : value_(value), refCounter_(refCounter)
{
    // Note: Cannot call shared_from_this() in constructor
    // Reference counting will be set up externally after construction
}

ArrayItem::~ArrayItem()
{
    // Note: Cannot call shared_from_this() in destructor
    // Reference counting cleanup should be done externally before destruction
}

void ArrayItem::InitializeReferences()
{
    if (refCounter_)
    {
        auto self = shared_from_this();
        for (const auto& item : value_)
        {
            if (item)
            {
                refCounter_->AddReference(item, self);
            }
        }
    }
}

StackItemType ArrayItem::GetType() const
{
    return StackItemType::Array;
}

bool ArrayItem::GetBoolean() const
{
    return true;
}

int64_t ArrayItem::GetInteger() const
{
    return 0;
}

std::vector<std::shared_ptr<StackItem>> ArrayItem::GetArray() const
{
    return value_;
}

std::vector<std::shared_ptr<StackItem>> ArrayItem::GetStruct() const
{
    return value_;
}

void ArrayItem::Add(std::shared_ptr<StackItem> item)
{
    if (refCounter_)
    {
        refCounter_->AddReference(item, shared_from_this());
    }
    value_.push_back(item);
}

std::shared_ptr<StackItem> ArrayItem::Get(size_t index) const
{
    if (index >= value_.size())
        throw std::out_of_range("Index out of range");

    return value_[index];
}

void ArrayItem::Set(size_t index, std::shared_ptr<StackItem> item)
{
    if (index >= value_.size())
        throw std::out_of_range("Index out of range");

    if (refCounter_)
    {
        refCounter_->RemoveReference(value_[index], shared_from_this());
        refCounter_->AddReference(item, shared_from_this());
    }

    value_[index] = item;
}

void ArrayItem::Remove(size_t index)
{
    if (index >= value_.size())
        throw std::out_of_range("Index out of range");

    if (refCounter_)
    {
        refCounter_->RemoveReference(value_[index], shared_from_this());
    }

    value_.erase(value_.begin() + index);
}

size_t ArrayItem::Size() const
{
    return value_.size();
}

void ArrayItem::Clear()
{
    if (refCounter_)
    {
        auto self = shared_from_this();
        for (const auto& item : value_)
        {
            refCounter_->RemoveReference(item, self);
        }
    }

    value_.clear();
}

bool ArrayItem::Equals(const StackItem& other) const
{
    return this == &other;
}

std::shared_ptr<StackItem> ArrayItem::DeepCopy(ReferenceCounter* refCounter, bool asImmutable) const
{
    std::vector<std::shared_ptr<StackItem>> newItems;
    newItems.reserve(value_.size());

    for (const auto& item : value_)
    {
        newItems.push_back(item->DeepCopy(refCounter, asImmutable));
    }

    return std::make_shared<ArrayItem>(newItems, refCounter);
}

// StructItem implementation
StructItem::StructItem(const std::vector<std::shared_ptr<StackItem>>& value, ReferenceCounter* refCounter)
    : ArrayItem(value, refCounter)
{
}

StackItemType StructItem::GetType() const
{
    return StackItemType::Struct;
}

bool StructItem::GetBoolean() const
{
    return !value_.empty();
}

std::shared_ptr<StructItem> StructItem::Clone() const
{
    std::vector<std::shared_ptr<StackItem>> clonedValue;
    clonedValue.reserve(value_.size());

    for (const auto& item : value_)
    {
        if (item->GetType() == StackItemType::Struct)
        {
            auto structItem = std::dynamic_pointer_cast<StructItem>(item);
            clonedValue.push_back(structItem->Clone());
        }
        else
        {
            clonedValue.push_back(item);
        }
    }

    return std::make_shared<StructItem>(clonedValue, refCounter_);
}

std::shared_ptr<StackItem> StructItem::DeepCopy(ReferenceCounter* refCounter, bool asImmutable) const
{
    std::vector<std::shared_ptr<StackItem>> newItems;
    newItems.reserve(value_.size());

    for (const auto& item : value_)
    {
        newItems.push_back(item->DeepCopy(refCounter, asImmutable));
    }

    return std::make_shared<StructItem>(newItems, refCounter);
}

bool StructItem::Equals(const StackItem& other) const
{
    if (this == &other)
        return true;

    if (other.GetType() != StackItemType::Struct)
        return false;

    auto otherArray = other.GetArray();
    if (value_.size() != otherArray.size())
        return false;

    // Use a static thread_local set to track visited pairs to prevent infinite recursion
    static thread_local std::set<std::pair<const StackItem*, const StackItem*>> visitedPairs;

    // Check if we're already comparing this pair
    auto thisPair = std::make_pair(this, &other);
    auto reversePair = std::make_pair(&other, this);

    if (visitedPairs.count(thisPair) || visitedPairs.count(reversePair))
        return true;  // Assume equal if we're in a circular reference

    // Add this pair to the visited set
    visitedPairs.insert(thisPair);

    bool result = true;
    for (size_t i = 0; i < value_.size() && result; i++)
    {
        if (!value_[i]->Equals(*otherArray[i]))
            result = false;
    }

    // Remove this pair from the visited set
    visitedPairs.erase(thisPair);

    return result;
}

// MapItem implementation
MapItem::MapItem(const std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>, StackItemPtrComparator>& value,
                 ReferenceCounter* refCounter)
    : value_(value), refCounter_(refCounter)
{
    if (refCounter_)
    {
        auto self = shared_from_this();
        for (const auto& [key, val] : value_)
        {
            refCounter_->AddReference(key, self);
            refCounter_->AddReference(val, self);
        }
    }
}

MapItem::~MapItem()
{
    if (refCounter_)
    {
        auto self = shared_from_this();
        for (const auto& [key, val] : value_)
        {
            refCounter_->RemoveReference(key, self);
            refCounter_->RemoveReference(val, self);
        }
    }
}

StackItemType MapItem::GetType() const
{
    return StackItemType::Map;
}

bool MapItem::GetBoolean() const
{
    return true;
}

std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> MapItem::GetMap() const
{
    // GetMap() is not well-supported for MapItem due to comparison issues
    // This method should ideally not be used - use Get() for individual items
    throw std::runtime_error("GetMap() is not supported for MapItem - use GetSize() and Get() methods instead");
}

size_t MapItem::GetSize() const
{
    return value_.size();
}

std::optional<std::shared_ptr<StackItem>> MapItem::Get(const std::shared_ptr<StackItem>& key) const
{
    for (const auto& [k, v] : value_)
    {
        if (k->Equals(*key))
            return v;
    }

    return std::nullopt;
}

void MapItem::Set(const std::shared_ptr<StackItem>& key, const std::shared_ptr<StackItem>& value)
{
    if (refCounter_)
    {
        auto self = shared_from_this();
        refCounter_->AddReference(key, self);
        refCounter_->AddReference(value, self);
    }

    for (auto it = value_.begin(); it != value_.end(); ++it)
    {
        if (it->first->Equals(*key))
        {
            if (refCounter_)
            {
                auto self = shared_from_this();
                refCounter_->RemoveReference(it->first, self);
                refCounter_->RemoveReference(it->second, self);
            }

            it->second = value;
            return;
        }
    }

    value_.insert(std::make_pair(key, value));
}

void MapItem::Remove(const std::shared_ptr<StackItem>& key)
{
    for (auto it = value_.begin(); it != value_.end(); ++it)
    {
        if (it->first->Equals(*key))
        {
            if (refCounter_)
            {
                auto self = shared_from_this();
                refCounter_->RemoveReference(it->first, self);
                refCounter_->RemoveReference(it->second, self);
            }

            value_.erase(it);
            return;
        }
    }
}

size_t MapItem::Size() const
{
    return value_.size();
}

void MapItem::Clear()
{
    if (refCounter_)
    {
        auto self = shared_from_this();
        for (const auto& [key, val] : value_)
        {
            refCounter_->RemoveReference(key, self);
            refCounter_->RemoveReference(val, self);
        }
    }

    value_.clear();
}

bool MapItem::Equals(const StackItem& other) const
{
    return this == &other;
}

std::shared_ptr<StackItem> MapItem::DeepCopy(ReferenceCounter* refCounter, bool asImmutable) const
{
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>, StackItemPtrComparator> newMap;

    for (const auto& [key, val] : value_)
    {
        auto newKey = key->DeepCopy(refCounter, asImmutable);
        auto newVal = val->DeepCopy(refCounter, asImmutable);
        newMap.insert(std::make_pair(newKey, newVal));
    }

    return std::make_shared<MapItem>(newMap, refCounter);
}
}  // namespace neo::vm
