#include <neo/vm/stack_item.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/reference_counter.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace neo::vm
{
    // StackItem implementation
    int64_t StackItem::GetInteger() const
    {
        throw std::runtime_error("Not supported");
    }

    io::ByteVector StackItem::GetByteArray() const
    {
        throw std::runtime_error("Not supported");
    }

    std::string StackItem::GetString() const
    {
        throw std::runtime_error("Not supported");
    }

    std::vector<std::shared_ptr<StackItem>> StackItem::GetArray() const
    {
        throw std::runtime_error("Not supported");
    }

    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> StackItem::GetMap() const
    {
        throw std::runtime_error("Not supported");
    }

    std::shared_ptr<void> StackItem::GetInterface() const
    {
        throw std::runtime_error("Not supported");
    }

    size_t StackItem::Size() const
    {
        throw std::runtime_error("Not supported");
    }

    std::shared_ptr<StackItem> StackItem::ConvertTo(StackItemType type) const
    {
        if (type == GetType()) return std::const_pointer_cast<StackItem>(shared_from_this());
        if (type == StackItemType::Boolean) return StackItem::Create(GetBoolean());
        if (type == StackItemType::Integer && (GetType() == StackItemType::Boolean || GetType() == StackItemType::ByteString || GetType() == StackItemType::Buffer))
            return StackItem::Create(GetInteger());
        if ((type == StackItemType::ByteString || type == StackItemType::Buffer) &&
            (GetType() == StackItemType::Boolean || GetType() == StackItemType::Integer))
        {
            auto byteArray = GetByteArray();
            if (type == StackItemType::ByteString)
                return std::make_shared<ByteStringItem>(byteArray);
            else
                return std::make_shared<BufferItem>(byteArray);
        }

        throw InvalidCastException("Cannot convert " + std::to_string(static_cast<int>(GetType())) + " to " + std::to_string(static_cast<int>(type)));
    }

    size_t StackItem::GetHashCode() const
    {
        throw std::runtime_error("Not supported");
    }

    bool StackItem::operator==(const StackItem& other) const
    {
        return Equals(other);
    }

    bool StackItem::operator!=(const StackItem& other) const
    {
        return !Equals(other);
    }

    int StackItem::CompareTo(const std::shared_ptr<StackItem>& other) const
    {
        if (!other)
            return 1;

        if (this->GetType() != other->GetType())
            throw InvalidOperationException("Cannot compare different types");

        if (this->Equals(*other))
            return 0;

        // For numeric types, compare the values
        if (this->GetType() == StackItemType::Integer)
        {
            int64_t thisValue = this->GetInteger();
            int64_t otherValue = other->GetInteger();

            if (thisValue < otherValue)
                return -1;
            else
                return 1;
        }

        // For byte strings, compare lexicographically
        if (this->GetType() == StackItemType::ByteString || this->GetType() == StackItemType::Buffer)
        {
            auto thisBytes = this->GetByteArray();
            auto otherBytes = other->GetByteArray();

            size_t minLength = std::min(thisBytes.Size(), otherBytes.Size());

            for (size_t i = 0; i < minLength; i++)
            {
                if (thisBytes[i] < otherBytes[i])
                    return -1;
                if (thisBytes[i] > otherBytes[i])
                    return 1;
            }

            if (thisBytes.Size() < otherBytes.Size())
                return -1;
            if (thisBytes.Size() > otherBytes.Size())
                return 1;

            return 0;
        }

        // For boolean values
        if (this->GetType() == StackItemType::Boolean)
        {
            bool thisValue = this->GetBoolean();
            bool otherValue = other->GetBoolean();

            if (thisValue == otherValue)
                return 0;
            if (!thisValue && otherValue)
                return -1;
            return 1;
        }

        throw InvalidOperationException("Cannot compare this type");
    }

    std::shared_ptr<StackItem> StackItem::DeepCopy(ReferenceCounter* refCounter, bool asImmutable) const
    {
        // Create a new shared_ptr with the same pointer
        return std::const_pointer_cast<StackItem>(shared_from_this());
    }

    bool StackItem::IsNull() const
    {
        return GetType() == StackItemType::Any;
    }

    bool StackItem::IsInterop() const
    {
        return GetType() == StackItemType::InteropInterface;
    }

    bool StackItem::IsArray() const
    {
        return GetType() == StackItemType::Array || GetType() == StackItemType::Struct;
    }

    bool StackItem::IsStruct() const
    {
        return GetType() == StackItemType::Struct;
    }

    std::shared_ptr<StackItem> StackItem::Null()
    {
        static std::shared_ptr<StackItem> nullItem = std::make_shared<NullItem>();
        return nullItem;
    }

    std::shared_ptr<StackItem> StackItem::True()
    {
        static std::shared_ptr<StackItem> trueItem = std::make_shared<BooleanItem>(true);
        return trueItem;
    }

    std::shared_ptr<StackItem> StackItem::False()
    {
        static std::shared_ptr<StackItem> falseItem = std::make_shared<BooleanItem>(false);
        return falseItem;
    }

    std::shared_ptr<StackItem> StackItem::Create(bool value)
    {
        return value ? True() : False();
    }

    std::shared_ptr<StackItem> StackItem::Create(int64_t value)
    {
        return std::make_shared<IntegerItem>(value);
    }

    std::shared_ptr<StackItem> StackItem::Create(const io::ByteVector& value)
    {
        return std::make_shared<ByteStringItem>(value);
    }

    std::shared_ptr<StackItem> StackItem::Create(const io::ByteSpan& value)
    {
        return std::make_shared<ByteStringItem>(value);
    }

    std::shared_ptr<StackItem> StackItem::Create(const std::string& value)
    {
        return std::make_shared<ByteStringItem>(io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size())));
    }

    std::shared_ptr<StackItem> StackItem::Create(const io::UInt160& value)
    {
        io::ByteVector bytes(20);
        std::memcpy(bytes.Data(), value.Data(), 20);
        return std::make_shared<ByteStringItem>(bytes);
    }

    std::shared_ptr<StackItem> StackItem::Create(const io::UInt256& value)
    {
        io::ByteVector bytes(32);
        std::memcpy(bytes.Data(), value.Data(), 32);
        return std::make_shared<ByteStringItem>(bytes);
    }

    std::shared_ptr<StackItem> StackItem::Create(const std::vector<std::shared_ptr<StackItem>>& value)
    {
        return std::make_shared<ArrayItem>(value, nullptr);
    }

    std::shared_ptr<StackItem> StackItem::CreateArray()
    {
        return std::make_shared<ArrayItem>(std::vector<std::shared_ptr<StackItem>>{}, nullptr);
    }

    std::shared_ptr<StackItem> StackItem::CreateArray(const std::vector<std::shared_ptr<StackItem>>& items)
    {
        return std::make_shared<ArrayItem>(items, nullptr);
    }

    void StackItem::Reset()
    {
        dfn_ = -1;
        low_link_ = -1;
        on_stack_ = false;
    }

    int StackItem::GetDFN() const
    {
        return dfn_;
    }

    void StackItem::SetDFN(int dfn)
    {
        dfn_ = dfn;
    }

    int StackItem::GetLowLink() const
    {
        return low_link_;
    }

    void StackItem::SetLowLink(int low_link)
    {
        low_link_ = low_link;
    }

    bool StackItem::IsOnStack() const
    {
        return on_stack_;
    }

    void StackItem::SetOnStack(bool on_stack)
    {
        on_stack_ = on_stack;
    }









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

    // InteropInterfaceItem implementation
    InteropInterfaceItem::InteropInterfaceItem(std::shared_ptr<void> value)
        : value_(value)
    {
    }

    StackItemType InteropInterfaceItem::GetType() const
    {
        return StackItemType::InteropInterface;
    }

    bool InteropInterfaceItem::GetBoolean() const
    {
        return value_ != nullptr;
    }

    std::shared_ptr<void> InteropInterfaceItem::GetInterface() const
    {
        return value_;
    }

    size_t InteropInterfaceItem::Size() const
    {
        return sizeof(void*);
    }

    size_t InteropInterfaceItem::GetHashCode() const
    {
        return std::hash<void*>()(value_.get());
    }

    bool InteropInterfaceItem::Equals(const StackItem& other) const
    {
        if (other.GetType() != StackItemType::InteropInterface)
            return false;

        auto otherInterface = other.GetInterface();
        return value_.get() == otherInterface.get();
    }

    // PointerItem implementation
    PointerItem::PointerItem(int32_t position, std::shared_ptr<StackItem> value)
        : position_(position), value_(value)
    {
    }

    StackItemType PointerItem::GetType() const
    {
        return StackItemType::Pointer;
    }

    bool PointerItem::GetBoolean() const
    {
        return true;
    }

    int32_t PointerItem::GetPosition() const
    {
        return position_;
    }

    std::shared_ptr<StackItem> PointerItem::GetValue() const
    {
        return value_;
    }

    bool PointerItem::Equals(const StackItem& other) const
    {
        if (other.GetType() != StackItemType::Pointer)
            return false;

        auto otherPointer = dynamic_cast<const PointerItem&>(other);
        return position_ == otherPointer.GetPosition() &&
               ((value_ == nullptr && otherPointer.GetValue() == nullptr) ||
                (value_ != nullptr && otherPointer.GetValue() != nullptr && value_->Equals(*otherPointer.GetValue())));
    }

    // NullItem implementation
    StackItemType NullItem::GetType() const
    {
        return StackItemType::Any;
    }

    bool NullItem::GetBoolean() const
    {
        return false;
    }

    bool NullItem::Equals(const StackItem& other) const
    {
        return other.GetType() == StackItemType::Any;
    }

    // Additional StackItem static methods
    std::shared_ptr<StackItem> StackItem::CreateStruct(ReferenceCounter& refCounter)
    {
        return std::make_shared<StructItem>(std::vector<std::shared_ptr<StackItem>>{}, &refCounter);
    }

    std::shared_ptr<StackItem> StackItem::CreateStruct(const std::vector<std::shared_ptr<StackItem>>& items, ReferenceCounter& refCounter)
    {
        return std::make_shared<StructItem>(items, &refCounter);
    }

    std::vector<std::shared_ptr<StackItem>> StackItem::GetStruct() const
    {
        if (GetType() == StackItemType::Struct || GetType() == StackItemType::Array)
        {
            return GetArray();
        }
        throw std::runtime_error("Item is not a struct or array");
    }

    void StackItem::Add(std::shared_ptr<StackItem> item)
    {
        throw std::runtime_error("Add not supported for this type");
    }

    std::shared_ptr<StackItem> StackItem::Deserialize(io::BinaryReader& reader)
    {
        // Basic deserialization - this should be expanded based on C# implementation
        uint8_t type = reader.ReadByte();
        
        switch (static_cast<StackItemType>(type))
        {
            case StackItemType::Boolean:
            {
                bool value = reader.ReadBoolean();
                return Create(value);
            }
            case StackItemType::Integer:
            {
                int64_t value = reader.ReadInt64();
                return Create(value);
            }
            case StackItemType::ByteString:
            {
                uint32_t length = reader.ReadVarInt();
                io::ByteVector data = reader.ReadBytes(length);
                return Create(data);
            }
            case StackItemType::Array:
            {
                uint32_t count = reader.ReadVarInt();
                std::vector<std::shared_ptr<StackItem>> items;
                items.reserve(count);
                
                for (uint32_t i = 0; i < count; i++)
                {
                    items.push_back(Deserialize(reader));
                }
                
                return CreateArray(items);
            }
            case StackItemType::Any:
                return Null();
            default:
                throw std::runtime_error("Unknown or unsupported stack item type");
        }
    }

    void StackItem::Serialize(std::shared_ptr<StackItem> item, io::BinaryWriter& writer)
    {
        if (!item)
        {
            writer.WriteByte(static_cast<uint8_t>(StackItemType::Any));
            return;
        }

        writer.WriteByte(static_cast<uint8_t>(item->GetType()));

        switch (item->GetType())
        {
            case StackItemType::Boolean:
                writer.WriteBoolean(item->GetBoolean());
                break;
            case StackItemType::Integer:
                writer.WriteInt64(item->GetInteger());
                break;
            case StackItemType::ByteString:
            case StackItemType::Buffer:
            {
                auto data = item->GetByteArray();
                writer.WriteVarInt(static_cast<uint32_t>(data.Size()));
                writer.WriteBytes(data);
                break;
            }
            case StackItemType::Array:
            case StackItemType::Struct:
            {
                auto array = item->GetArray();
                writer.WriteVarInt(static_cast<uint32_t>(array.size()));
                
                for (const auto& element : array)
                {
                    Serialize(element, writer);
                }
                break;
            }
            case StackItemType::Any:
                // Null item - already wrote the type
                break;
            default:
                throw std::runtime_error("Unsupported stack item type for serialization");
        }
    }
}
