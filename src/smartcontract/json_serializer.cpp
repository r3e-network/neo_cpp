/**
 * @file json_serializer.cpp
 * @brief JSON serialization utilities
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/base64.h>
#include <neo/smartcontract/json_serializer.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/stack_item_types.h>

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace neo::smartcontract
{
nlohmann::json JsonSerializer::Serialize(std::shared_ptr<vm::StackItem> item)
{
    int64_t itemCount = 0;
    return SerializeStackItem(item, itemCount);
}

io::ByteVector JsonSerializer::SerializeToByteArray(std::shared_ptr<vm::StackItem> item, int64_t maxSize)
{
    auto json = Serialize(item);
    std::string jsonStr = json.dump();

    if (static_cast<int64_t>(jsonStr.size()) > maxSize)
        throw std::runtime_error("Serialized JSON exceeds maximum size");

    return io::ByteVector(reinterpret_cast<const uint8_t*>(jsonStr.data()), jsonStr.size());
}

std::shared_ptr<vm::StackItem> JsonSerializer::Deserialize(const nlohmann::json& json, int64_t maxSize,
                                                           int64_t maxItems)
{
    int64_t itemCount = 0;
    return DeserializeStackItem(json, maxSize, maxItems, itemCount);
}

std::shared_ptr<vm::StackItem> JsonSerializer::Deserialize(const io::ByteSpan& data, int64_t maxSize, int64_t maxItems)
{
    if (static_cast<int64_t>(data.Size()) > maxSize) throw std::runtime_error("Data exceeds maximum size");

    std::string jsonStr(reinterpret_cast<const char*>(data.Data()), data.Size());
    auto json = nlohmann::json::parse(jsonStr);
    return Deserialize(json, maxSize, maxItems);
}

nlohmann::json JsonSerializer::SerializeStackItem(std::shared_ptr<vm::StackItem> item, int64_t& itemCount)
{
    if (++itemCount > 2048)  // Default max items
        throw std::runtime_error("Maximum item count exceeded");

    if (!item) return nlohmann::json::value_t::null;

    // Serialize based on stack item type
    switch (item->GetType())
    {
        case vm::StackItemType::Null:
            return nlohmann::json::value_t::null;

        case vm::StackItemType::Boolean:
            return item->GetBoolean();

        case vm::StackItemType::Integer:
            return item->GetInteger();

        case vm::StackItemType::ByteString:
        case vm::StackItemType::Buffer:
        {
            auto bytes = item->GetByteArray();
            return cryptography::Base64::Encode(bytes.AsSpan());
        }

        case vm::StackItemType::Array:
        case vm::StackItemType::Struct:
        {
            auto array = item->GetArray();
            nlohmann::json jsonArray = nlohmann::json::array();
            for (const auto& element : array)
            {
                jsonArray.push_back(SerializeStackItem(element, itemCount));
            }
            return jsonArray;
        }

        case vm::StackItemType::Map:
        {
            auto map = item->GetMap();
            nlohmann::json jsonObject = nlohmann::json::object();
            for (const auto& [key, value] : map)
            {
                // Use base64 encoded key for complex key types
                std::string keyStr;
                if (key->GetType() == vm::StackItemType::ByteString || key->GetType() == vm::StackItemType::Buffer)
                {
                    auto keyBytes = key->GetByteArray();
                    keyStr = cryptography::Base64::Encode(keyBytes.AsSpan());
                }
                else
                {
                    keyStr = key->GetString();
                }
                jsonObject[keyStr] = SerializeStackItem(value, itemCount);
            }
            return jsonObject;
        }

        case vm::StackItemType::InteropInterface:
            // InteropInterface items cannot be serialized to JSON
            throw std::runtime_error("Cannot serialize InteropInterface to JSON");

        case vm::StackItemType::Pointer:
            // Pointer items cannot be serialized to JSON
            throw std::runtime_error("Cannot serialize Pointer to JSON");

        default:
            throw std::runtime_error("Unknown stack item type");
    }
}

std::shared_ptr<vm::StackItem> JsonSerializer::DeserializeStackItem(const nlohmann::json& json, int64_t maxSize,
                                                                    int64_t maxItems, int64_t& itemCount)
{
    if (++itemCount > maxItems) throw std::runtime_error("Maximum item count exceeded");

    // Deserialize based on JSON type
    if (json.is_null())
    {
        return vm::StackItem::Null();
    }
    else if (json.is_boolean())
    {
        return vm::StackItem::Create(json.get<bool>());
    }
    else if (json.is_number_integer())
    {
        return vm::StackItem::Create(json.get<int64_t>());
    }
    else if (json.is_number_unsigned())
    {
        return vm::StackItem::Create(static_cast<int64_t>(json.get<uint64_t>()));
    }
    else if (json.is_number_float())
    {
        // Convert float to integer (truncate)
        return vm::StackItem::Create(static_cast<int64_t>(json.get<double>()));
    }
    else if (json.is_string())
    {
        std::string str = json.get<std::string>();

        // Try to decode as base64 first (for byte arrays)
        try
        {
            auto bytes = cryptography::Base64::Decode(str);
            return vm::StackItem::Create(bytes);
        }
        catch (const std::invalid_argument&)
        {
            // If base64 decoding fails, treat as regular string
            io::ByteVector bytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
            return vm::StackItem::Create(bytes);
        }
        catch (const std::runtime_error&)
        {
            // If base64 decoding fails, treat as regular string
            io::ByteVector bytes(reinterpret_cast<const uint8_t*>(str.data()), str.size());
            return vm::StackItem::Create(bytes);
        }
    }
    else if (json.is_array())
    {
        std::vector<std::shared_ptr<vm::StackItem>> array;
        for (const auto& element : json)
        {
            array.push_back(DeserializeStackItem(element, maxSize, maxItems, itemCount));
        }
        return vm::StackItem::Create(array);
    }
    else if (json.is_object())
    {
        // Create a map from JSON object
        std::map<std::shared_ptr<vm::StackItem>, std::shared_ptr<vm::StackItem>, vm::StackItemPtrComparator> map;

        for (auto it = json.begin(); it != json.end(); ++it)
        {
            // Create key from string
            io::ByteVector keyBytes(reinterpret_cast<const uint8_t*>(it.key().data()), it.key().size());
            auto key = vm::StackItem::Create(keyBytes);

            // Deserialize value
            auto value = DeserializeStackItem(it.value(), maxSize, maxItems, itemCount);

            map[key] = value;
        }

        return std::make_shared<vm::MapItem>(map);
    }
    else
    {
        throw std::runtime_error("Unsupported JSON type for deserialization");
    }
}
}  // namespace neo::smartcontract