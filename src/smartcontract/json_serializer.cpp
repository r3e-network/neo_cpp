#include <neo/smartcontract/json_serializer.h>
#include <neo/vm/stack_item_type.h>
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
        std::string jsonString = json.dump();
        
        if (static_cast<int64_t>(jsonString.size()) > maxSize)
            throw std::runtime_error("Serialized JSON exceeds maximum size");
            
        return io::ByteVector(io::ByteSpan(reinterpret_cast<const uint8_t*>(jsonString.data()), jsonString.size()));
    }

    std::shared_ptr<vm::StackItem> JsonSerializer::Deserialize(const nlohmann::json& json, int64_t maxSize, int64_t maxItems)
    {
        int64_t itemCount = 0;
        return DeserializeStackItem(json, maxSize, maxItems, itemCount);
    }

    std::shared_ptr<vm::StackItem> JsonSerializer::Deserialize(const io::ByteSpan& data, int64_t maxSize, int64_t maxItems)
    {
        if (data.Size() > maxSize)
            throw std::runtime_error("Data exceeds maximum size");
            
        std::string jsonString(reinterpret_cast<const char*>(data.Data()), data.Size());
        
        try
        {
            auto json = nlohmann::json::parse(jsonString);
            return Deserialize(json, maxSize, maxItems);
        }
        catch (const nlohmann::json::parse_error& e)
        {
            throw std::runtime_error("Invalid JSON format: " + std::string(e.what()));
        }
    }

    nlohmann::json JsonSerializer::SerializeStackItem(std::shared_ptr<vm::StackItem> item, int64_t& itemCount)
    {
        ++itemCount;

        switch (item->GetType())
        {
            case vm::StackItemType::Boolean:
                return nlohmann::json(item->GetBoolean());

            case vm::StackItemType::Integer:
                {
                    auto value = item->GetInteger();
                    // Check if the integer is within safe JSON range (like C# implementation)
                    if (value > MAX_SAFE_INTEGER || value < MIN_SAFE_INTEGER)
                        throw std::runtime_error("Integer value exceeds safe JSON range");
                    return nlohmann::json(static_cast<double>(value));
                }

            case vm::StackItemType::ByteString:
            case vm::StackItemType::Buffer:
                {
                    // Convert to string (like C# implementation)
                    auto data = item->GetByteArray();
                    std::string str(reinterpret_cast<const char*>(data.Data()), data.Size());
                    return nlohmann::json(str);
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
                    
                    for (const auto& pair : map)
                    {
                        // Convert key to string (like C# implementation)
                        std::string key;
                        if (pair.first->GetType() == vm::StackItemType::ByteString)
                        {
                            auto keyData = pair.first->GetByteArray();
                            key = std::string(reinterpret_cast<const char*>(keyData.Data()), keyData.Size());
                        }
                        else
                        {
                            // For non-string keys, convert to JSON and use as string
                            auto keyJson = SerializeStackItem(pair.first, itemCount);
                            key = keyJson.dump();
                        }
                        
                        jsonObject[key] = SerializeStackItem(pair.second, itemCount);
                    }
                    
                    return jsonObject;
                }

            case vm::StackItemType::Null:
                return nlohmann::json(nullptr);

            default:
                throw std::runtime_error("Unsupported stack item type for JSON serialization");
        }
    }

    std::shared_ptr<vm::StackItem> JsonSerializer::DeserializeStackItem(const nlohmann::json& json, 
                                                                       int64_t maxSize, int64_t maxItems, int64_t& itemCount)
    {
        if (++itemCount > maxItems)
            throw std::runtime_error("Too many items to deserialize");

        if (json.is_null())
        {
            return vm::StackItem::CreateNull();
        }
        else if (json.is_boolean())
        {
            return vm::StackItem::Create(json.get<bool>());
        }
        else if (json.is_number())
        {
            if (json.is_number_integer())
            {
                return vm::StackItem::Create(json.get<int64_t>());
            }
            else
            {
                // Convert double to integer (like C# implementation)
                double value = json.get<double>();
                if (value > MAX_SAFE_INTEGER || value < MIN_SAFE_INTEGER)
                    throw std::runtime_error("Number value exceeds safe range");
                return vm::StackItem::Create(static_cast<int64_t>(value));
            }
        }
        else if (json.is_string())
        {
            std::string str = json.get<std::string>();
            return vm::StackItem::Create(str);
        }
        else if (json.is_array())
        {
            std::vector<std::shared_ptr<vm::StackItem>> array;
            array.reserve(json.size());
            
            for (const auto& element : json)
            {
                array.push_back(DeserializeStackItem(element, maxSize, maxItems, itemCount));
            }
            
            return vm::StackItem::CreateArray(array);
        }
        else if (json.is_object())
        {
            std::map<std::shared_ptr<vm::StackItem>, std::shared_ptr<vm::StackItem>> map;
            
            for (auto it = json.begin(); it != json.end(); ++it)
            {
                auto key = vm::StackItem::Create(it.key());
                auto value = DeserializeStackItem(it.value(), maxSize, maxItems, itemCount);
                map[key] = value;
            }
            
            return vm::StackItem::CreateMap(map);
        }
        else
        {
            throw std::runtime_error("Unsupported JSON type for deserialization");
        }
    }
}
