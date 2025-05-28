#include <neo/smartcontract/binary_serializer.h>
#include <neo/vm/stack_item_type.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract
{
    io::ByteVector BinarySerializer::Serialize(std::shared_ptr<vm::StackItem> item, int64_t maxSize, int64_t maxItems)
    {
        std::ostringstream stream;
        io::BinaryWriter writer(stream);
        int64_t itemCount = 0;
        
        SerializeStackItem(writer, item, maxSize, maxItems, itemCount);
        
        std::string str = stream.str();
        io::ByteVector result(io::ByteSpan(reinterpret_cast<const uint8_t*>(str.data()), str.size()));
        
        if (result.Size() > maxSize)
            throw std::runtime_error("Serialized data exceeds maximum size");
            
        return result;
    }

    std::shared_ptr<vm::StackItem> BinarySerializer::Deserialize(const io::ByteSpan& data, int64_t maxSize, int64_t maxItems)
    {
        if (data.Size() > maxSize)
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
            throw std::runtime_error("Too many items to serialize");

        // Write the stack item type (exactly like C# implementation)
        writer.Write(static_cast<uint8_t>(item->GetType()));

        switch (item->GetType())
        {
            case vm::StackItemType::Boolean:
                writer.Write(item->GetBoolean() ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0));
                break;

            case vm::StackItemType::Integer:
                {
                    auto value = item->GetInteger();
                    // Convert to bytes in little-endian format (like C# BigInteger.ToByteArray)
                    std::vector<uint8_t> bytes;
                    if (value == 0)
                    {
                        bytes.push_back(0);
                    }
                    else
                    {
                        bool negative = value < 0;
                        if (negative) value = -value;
                        
                        while (value > 0)
                        {
                            bytes.push_back(static_cast<uint8_t>(value & 0xFF));
                            value >>= 8;
                        }
                        
                        if (negative)
                        {
                            // Two's complement
                            for (size_t i = 0; i < bytes.size(); ++i)
                            {
                                bytes[i] = ~bytes[i];
                            }
                            // Add 1
                            bool carry = true;
                            for (size_t i = 0; i < bytes.size() && carry; ++i)
                            {
                                bytes[i]++;
                                carry = (bytes[i] == 0);
                            }
                            if (carry)
                                bytes.push_back(0xFF);
                        }
                    }
                    
                    writer.WriteVarBytes(io::ByteSpan(bytes.data(), bytes.size()));
                }
                break;

            case vm::StackItemType::ByteString:
            case vm::StackItemType::Buffer:
                {
                    auto data = item->GetByteArray();
                    writer.WriteVarBytes(data.AsSpan());
                }
                break;

            case vm::StackItemType::Array:
            case vm::StackItemType::Struct:
                {
                    auto array = item->GetArray();
                    writer.WriteVarInt(static_cast<int64_t>(array.size()));
                    for (const auto& element : array)
                    {
                        SerializeStackItem(writer, element, maxSize, maxItems, itemCount);
                    }
                }
                break;

            case vm::StackItemType::Map:
                {
                    auto map = item->GetMap();
                    writer.WriteVarInt(static_cast<int64_t>(map.size()));
                    for (const auto& pair : map)
                    {
                        SerializeStackItem(writer, pair.first, maxSize, maxItems, itemCount);
                        SerializeStackItem(writer, pair.second, maxSize, maxItems, itemCount);
                    }
                }
                break;

            default:
                throw std::runtime_error("Unsupported stack item type for serialization");
        }
    }

    std::shared_ptr<vm::StackItem> BinarySerializer::DeserializeStackItem(io::BinaryReader& reader, 
                                                                         int64_t maxSize, int64_t maxItems, int64_t& itemCount)
    {
        if (++itemCount > maxItems)
            throw std::runtime_error("Too many items to deserialize");

        auto type = static_cast<vm::StackItemType>(reader.ReadByte());

        switch (type)
        {
            case vm::StackItemType::Boolean:
                return vm::StackItem::Create(reader.ReadByte() != 0);

            case vm::StackItemType::Integer:
                {
                    auto bytes = reader.ReadVarBytes();
                    if (bytes.Size() == 0)
                        return vm::StackItem::Create(static_cast<int64_t>(0));
                    
                    // Convert from little-endian bytes to integer
                    int64_t value = 0;
                    bool negative = (bytes[bytes.Size() - 1] & 0x80) != 0;
                    
                    for (size_t i = 0; i < bytes.Size() && i < 8; ++i)
                    {
                        value |= static_cast<int64_t>(bytes[i]) << (i * 8);
                    }
                    
                    return vm::StackItem::Create(value);
                }

            case vm::StackItemType::ByteString:
                {
                    auto data = reader.ReadVarBytes();
                    return vm::StackItem::Create(data);
                }

            case vm::StackItemType::Buffer:
                {
                    auto data = reader.ReadVarBytes();
                    return vm::StackItem::CreateBuffer(data);
                }

            case vm::StackItemType::Array:
                {
                    auto count = reader.ReadVarInt();
                    std::vector<std::shared_ptr<vm::StackItem>> array;
                    array.reserve(static_cast<size_t>(count));
                    
                    for (int64_t i = 0; i < count; ++i)
                    {
                        array.push_back(DeserializeStackItem(reader, maxSize, maxItems, itemCount));
                    }
                    
                    return vm::StackItem::CreateArray(array);
                }

            case vm::StackItemType::Struct:
                {
                    auto count = reader.ReadVarInt();
                    std::vector<std::shared_ptr<vm::StackItem>> array;
                    array.reserve(static_cast<size_t>(count));
                    
                    for (int64_t i = 0; i < count; ++i)
                    {
                        array.push_back(DeserializeStackItem(reader, maxSize, maxItems, itemCount));
                    }
                    
                    return vm::StackItem::CreateStruct(array);
                }

            case vm::StackItemType::Map:
                {
                    auto count = reader.ReadVarInt();
                    std::map<std::shared_ptr<vm::StackItem>, std::shared_ptr<vm::StackItem>> map;
                    
                    for (int64_t i = 0; i < count; ++i)
                    {
                        auto key = DeserializeStackItem(reader, maxSize, maxItems, itemCount);
                        auto value = DeserializeStackItem(reader, maxSize, maxItems, itemCount);
                        map[key] = value;
                    }
                    
                    return vm::StackItem::CreateMap(map);
                }

            default:
                throw std::runtime_error("Unsupported stack item type for deserialization");
        }
    }
}
