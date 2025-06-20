#include <neo/vm/compound_items.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/exceptions.h>
#include <stdexcept>
#include <algorithm>

namespace neo::vm
{
    // ArrayItem implementation
    ArrayItem::ArrayItem(const std::vector<std::shared_ptr<StackItem>>& value, ReferenceCounter* refCounter)
        : value_(value), refCounter_(refCounter)
    {
        if (refCounter_)
        {
            auto self = shared_from_this();
            for (const auto& item : value_)
            {
                refCounter_->AddReference(item, self);
            }
        }
    }

    ArrayItem::~ArrayItem()
    {
        if (refCounter_)
        {
            auto self = shared_from_this();
            for (const auto& item : value_)
            {
                refCounter_->RemoveReference(item, self);
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

    std::vector<std::shared_ptr<StackItem>> ArrayItem::GetArray() const
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

        for (size_t i = 0; i < value_.size(); i++)
        {
            if (!value_[i]->Equals(*otherArray[i]))
                return false;
        }

        return true;
    }

    // MapItem implementation
    MapItem::MapItem(const std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>>& value, ReferenceCounter* refCounter)
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
        return value_;
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

        value_[key] = value;
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
        std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> newMap;

        for (const auto& [key, val] : value_)
        {
            auto newKey = key->DeepCopy(refCounter, asImmutable);
            auto newVal = val->DeepCopy(refCounter, asImmutable);
            newMap[newKey] = newVal;
        }

        return std::make_shared<MapItem>(newMap, refCounter);
    }
}
