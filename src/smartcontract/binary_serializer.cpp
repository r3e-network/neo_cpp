#include <neo/smartcontract/binary_serializer.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/stack_item_types.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract
{
io::ByteVector BinarySerializer::Serialize(std::shared_ptr<vm::StackItem> item, int64_t maxSize, int64_t maxItems)
{
    // Serialize the stack item to binary format
    std::ostringstream stream;
    io::BinaryWriter writer(stream);

    int64_t itemCount = 0;
    SerializeStackItem(writer, item, maxSize, maxItems, itemCount);

    std::string str = stream.str();
    io::ByteVector result(io::ByteSpan(reinterpret_cast<const uint8_t*>(str.data()), str.size()));

    if (static_cast<int64_t>(result.Size()) > maxSize)
        throw std::runtime_error("Serialized data exceeds maximum size");

    return result;
}

std::shared_ptr<vm::StackItem> BinarySerializer::Deserialize(const io::ByteSpan& data, int64_t maxSize,
                                                             int64_t maxItems)
{
    if (static_cast<int64_t>(data.Size()) > maxSize)
        throw std::runtime_error("Data exceeds maximum size");

    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.Data()), data.Size()));
    io::BinaryReader reader(stream);

    int64_t itemCount = 0;
    return DeserializeStackItem(reader, maxSize, maxItems, itemCount);
}

void BinarySerializer::SerializeStackItem(io::BinaryWriter& writer, std::shared_ptr<vm::StackItem> item,
                                          int64_t maxSize, int64_t maxItems, int64_t& itemCount)
{
    if (++itemCount > maxItems)
        throw std::runtime_error("Maximum item count exceeded");

    if (!item)
    {
        writer.WriteByte(static_cast<uint8_t>(vm::StackItemType::Any));
        return;
    }

    writer.WriteByte(static_cast<uint8_t>(item->GetType()));

    // Serialize based on type
    switch (item->GetType())
    {
        case vm::StackItemType::Any:
        case vm::StackItemType::Null:
            // No additional data needed
            break;

        case vm::StackItemType::Boolean:
            writer.WriteByte(item->GetBoolean() ? 1 : 0);
            break;

        case vm::StackItemType::Integer:
        {
            auto value = item->GetInteger();
            writer.WriteVarInt(value);
            break;
        }

        case vm::StackItemType::ByteString:
        case vm::StackItemType::Buffer:
        {
            auto bytes = item->GetByteArray();
            writer.WriteVarBytes(bytes.AsSpan());
            break;
        }

        case vm::StackItemType::Array:
        case vm::StackItemType::Struct:
        {
            auto array = item->GetArray();
            writer.WriteVarInt(static_cast<uint64_t>(array.size()));
            for (const auto& element : array)
            {
                SerializeStackItem(writer, element, maxSize, maxItems, itemCount);
            }
            break;
        }

        case vm::StackItemType::Map:
        {
            auto map = item->GetMap();
            writer.WriteVarInt(static_cast<uint64_t>(map.size()));
            for (const auto& [key, value] : map)
            {
                SerializeStackItem(writer, key, maxSize, maxItems, itemCount);
                SerializeStackItem(writer, value, maxSize, maxItems, itemCount);
            }
            break;
        }

        case vm::StackItemType::InteropInterface:
        case vm::StackItemType::Pointer:
            throw std::runtime_error("Cannot serialize InteropInterface or Pointer types");

        default:
            throw std::runtime_error("Unknown stack item type");
    }
}

std::shared_ptr<vm::StackItem> BinarySerializer::DeserializeStackItem(io::BinaryReader& reader, int64_t maxSize,
                                                                      int64_t maxItems, int64_t& itemCount)
{
    if (++itemCount > maxItems)
        throw std::runtime_error("Maximum item count exceeded");

    auto type = static_cast<vm::StackItemType>(reader.ReadByte());

    // Deserialize based on type
    switch (type)
    {
        case vm::StackItemType::Any:
        case vm::StackItemType::Null:
            return vm::StackItem::Null();

        case vm::StackItemType::Boolean:
            return vm::StackItem::Create(reader.ReadByte() != 0);

        case vm::StackItemType::Integer:
        {
            auto value = reader.ReadVarInt();
            return vm::StackItem::Create(static_cast<int64_t>(value));
        }

        case vm::StackItemType::ByteString:
        case vm::StackItemType::Buffer:
        {
            auto bytes = reader.ReadVarBytes(maxSize);
            return vm::StackItem::Create(bytes);
        }

        case vm::StackItemType::Array:
        case vm::StackItemType::Struct:
        {
            auto count = reader.ReadVarInt();
            if (static_cast<int64_t>(count) > maxItems - itemCount)
                throw std::runtime_error("Array size exceeds maximum items");

            std::vector<std::shared_ptr<vm::StackItem>> array;
            array.reserve(count);

            for (uint64_t i = 0; i < count; ++i)
            {
                array.push_back(DeserializeStackItem(reader, maxSize, maxItems, itemCount));
            }

            // Both Array and Struct use the same creation method
            return vm::StackItem::Create(array);
        }

        case vm::StackItemType::Map:
        {
            auto count = reader.ReadVarInt();
            if (static_cast<int64_t>(count) > (maxItems - itemCount) / 2)
                throw std::runtime_error("Map size exceeds maximum items");

            std::map<std::shared_ptr<vm::StackItem>, std::shared_ptr<vm::StackItem>, vm::StackItemPtrComparator> map;

            for (uint64_t i = 0; i < count; ++i)
            {
                auto key = DeserializeStackItem(reader, maxSize, maxItems, itemCount);
                auto value = DeserializeStackItem(reader, maxSize, maxItems, itemCount);
                map[key] = value;
            }

            return std::make_shared<vm::MapItem>(map);
        }

        case vm::StackItemType::InteropInterface:
        case vm::StackItemType::Pointer:
            throw std::runtime_error("Cannot deserialize InteropInterface or Pointer types");

        default:
            throw std::runtime_error("Unknown stack item type");
    }
}
}  // namespace neo::smartcontract