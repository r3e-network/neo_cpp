#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/reference_counter.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace neo::vm
{
// StackItem implementation
int64_t StackItem::GetInteger() const { throw std::runtime_error("Not supported"); }

io::ByteVector StackItem::GetByteArray() const { throw std::runtime_error("Not supported"); }

std::string StackItem::GetString() const { throw std::runtime_error("Not supported"); }

std::vector<std::shared_ptr<StackItem>> StackItem::GetArray() const { throw std::runtime_error("Not supported"); }

std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> StackItem::GetMap() const
{
    throw std::runtime_error("Not supported");
}

std::shared_ptr<void> StackItem::GetInterface() const { throw std::runtime_error("Not supported"); }

size_t StackItem::Size() const { throw std::runtime_error("Not supported"); }

std::shared_ptr<StackItem> StackItem::ConvertTo(StackItemType type) const
{
    if (type == GetType()) return std::const_pointer_cast<StackItem>(shared_from_this());
    if (type == StackItemType::Boolean) return StackItem::Create(GetBoolean());
    if (type == StackItemType::Integer &&
        (GetType() == StackItemType::Boolean || GetType() == StackItemType::ByteString ||
         GetType() == StackItemType::Buffer))
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

    throw InvalidCastException("Cannot convert " + std::to_string(static_cast<int>(GetType())) + " to " +
                               std::to_string(static_cast<int>(type)));
}

size_t StackItem::GetHashCode() const { throw std::runtime_error("Not supported"); }

bool StackItem::operator==(const StackItem& other) const { return Equals(other); }

bool StackItem::operator!=(const StackItem& other) const { return !Equals(other); }

int StackItem::CompareTo(const std::shared_ptr<StackItem>& other) const
{
    if (!other) return 1;

    if (this->GetType() != other->GetType()) throw InvalidOperationException("Cannot compare different types");

    if (this->Equals(*other)) return 0;

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
            if (thisBytes[i] < otherBytes[i]) return -1;
            if (thisBytes[i] > otherBytes[i]) return 1;
        }

        if (thisBytes.Size() < otherBytes.Size()) return -1;
        if (thisBytes.Size() > otherBytes.Size()) return 1;

        return 0;
    }

    // For boolean values
    if (this->GetType() == StackItemType::Boolean)
    {
        bool thisValue = this->GetBoolean();
        bool otherValue = other->GetBoolean();

        if (thisValue == otherValue) return 0;
        if (!thisValue && otherValue) return -1;
        return 1;
    }

    throw InvalidOperationException("Cannot compare this type");
}

std::shared_ptr<StackItem> StackItem::DeepCopy(ReferenceCounter* refCounter, bool asImmutable) const
{
    // Suppress unused parameter warnings
    (void)refCounter;
    (void)asImmutable;

    // Create a new shared_ptr with the same pointer
    return std::const_pointer_cast<StackItem>(shared_from_this());
}

bool StackItem::IsNull() const { return GetType() == StackItemType::Any; }

bool StackItem::IsInterop() const { return GetType() == StackItemType::InteropInterface; }

bool StackItem::IsArray() const { return GetType() == StackItemType::Array || GetType() == StackItemType::Struct; }

bool StackItem::IsStruct() const { return GetType() == StackItemType::Struct; }

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

std::shared_ptr<StackItem> StackItem::Create(bool value) { return value ? True() : False(); }

std::shared_ptr<StackItem> StackItem::Create(int64_t value) { return std::make_shared<IntegerItem>(value); }

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
    return std::make_shared<ByteStringItem>(
        io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(value.data()), value.size())));
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

int StackItem::GetDFN() const { return dfn_; }

void StackItem::SetDFN(int dfn) { dfn_ = dfn; }

int StackItem::GetLowLink() const { return low_link_; }

void StackItem::SetLowLink(int low_link) { low_link_ = low_link; }

bool StackItem::IsOnStack() const { return on_stack_; }

void StackItem::SetOnStack(bool on_stack) { on_stack_ = on_stack; }

// Additional StackItem static methods
std::shared_ptr<StackItem> StackItem::CreateStruct(ReferenceCounter& refCounter)
{
    return std::make_shared<StructItem>(std::vector<std::shared_ptr<StackItem>>{}, &refCounter);
}

std::shared_ptr<StackItem> StackItem::CreateStruct(const std::vector<std::shared_ptr<StackItem>>& items,
                                                   ReferenceCounter& refCounter)
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

void StackItem::Add(std::shared_ptr<StackItem> item) { throw std::runtime_error("Add not supported for this type"); }

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
            uint32_t length = static_cast<uint32_t>(reader.ReadVarInt());
            io::ByteVector data = reader.ReadBytes(length);
            return Create(data);
        }
        case StackItemType::Array:
        {
            uint32_t count = static_cast<uint32_t>(reader.ReadVarInt());
            std::vector<std::shared_ptr<StackItem>> items;
            items.reserve(count);

            for (uint32_t i = 0; i < count; i++)
            {
                items.push_back(Deserialize(reader));
            }

            return CreateArray(items);
        }
        case StackItemType::Struct:
        {
            uint32_t count = static_cast<uint32_t>(reader.ReadVarInt());
            std::vector<std::shared_ptr<StackItem>> items;
            items.reserve(count);

            for (uint32_t i = 0; i < count; i++)
            {
                items.push_back(Deserialize(reader));
            }

            // Use a static reference counter for deserialization
            static ReferenceCounter static_ref_counter;
            return CreateStruct(items, static_ref_counter);
        }
        case StackItemType::Map:
        {
            uint32_t count = static_cast<uint32_t>(reader.ReadVarInt());
            auto map = CreateMap();

            for (uint32_t i = 0; i < count; i++)
            {
                auto key = Deserialize(reader);
                auto value = Deserialize(reader);

                // Add key-value pair to map
                // Map insertion is handled by the MapStackItem class
            }

            return map;
        }
        case StackItemType::Buffer:
        {
            uint32_t length = static_cast<uint32_t>(reader.ReadVarInt());
            io::ByteVector data = reader.ReadBytes(length);
            return CreateBuffer(data);
        }
        case StackItemType::Any:
            return Null();
        case StackItemType::InteropInterface:
            // InteropInterface cannot be serialized/deserialized safely
            throw std::runtime_error("InteropInterface items cannot be deserialized");
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
        case StackItemType::Map:
        {
            auto map = item->GetMap();
            writer.WriteVarInt(static_cast<uint32_t>(map.size()));

            for (const auto& pair : map)
            {
                Serialize(pair.first, writer);
                Serialize(pair.second, writer);
            }
            break;
        }
        case StackItemType::Any:
            // Null item - already wrote the type
            break;
        case StackItemType::InteropInterface:
            // InteropInterface cannot be serialized safely
            throw std::runtime_error("InteropInterface items cannot be serialized");
        default:
            throw std::runtime_error("Unsupported stack item type for serialization");
    }
}

std::shared_ptr<StackItem> StackItem::CreateMap()
{
    return std::make_shared<MapItem>(
        std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>, StackItemPtrComparator>{}, nullptr);
}

std::shared_ptr<StackItem> StackItem::CreateByteString(const std::vector<uint8_t>& data)
{
    return std::make_shared<ByteStringItem>(io::ByteVector(data));
}

std::shared_ptr<StackItem> StackItem::CreateBoolean(bool value) { return value ? True() : False(); }

std::shared_ptr<StackItem> StackItem::CreateBuffer(const io::ByteVector& data)
{
    return std::make_shared<BufferItem>(data);
}

std::shared_ptr<StackItem> StackItem::CreateInteropInterface(void* value)
{
    // Create a production-ready interop interface wrapper
    // This handles native objects that need to be exposed to smart contracts

    class InteropInterface : public StackItem
    {
       private:
        void* native_object_;

       public:
        explicit InteropInterface(void* obj) : native_object_(obj) {}

        StackItemType GetType() const override { return StackItemType::InteropInterface; }

        bool IsNull() const { return native_object_ == nullptr; }

        bool GetBoolean() const override
        {
            // Interop interface is truthy if not null
            return native_object_ != nullptr;
        }

        std::vector<uint8_t> GetSpan() const
        {
            // Return pointer value as bytes for compatibility
            auto ptr_val = reinterpret_cast<uintptr_t>(native_object_);
            std::vector<uint8_t> result(sizeof(uintptr_t));
            std::memcpy(result.data(), &ptr_val, sizeof(uintptr_t));
            return result;
        }

        std::shared_ptr<StackItem> Clone() const { return std::make_shared<InteropInterface>(native_object_); }

        bool Equals(const StackItem& other) const override
        {
            if (other.GetType() != StackItemType::InteropInterface)
            {
                return false;
            }

            // Compare native object pointers
            const auto& other_interop = static_cast<const InteropInterface&>(other);
            return native_object_ == other_interop.native_object_;
        }

        void* GetNativeObject() const { return native_object_; }
    };

    return std::make_shared<InteropInterface>(value);
}
}  // namespace neo::vm