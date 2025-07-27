#include <neo/cryptography/base64.h>
#include <neo/smartcontract/json_serializer.h>
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
    if (static_cast<int64_t>(data.Size()) > maxSize)
        throw std::runtime_error("Data exceeds maximum size");

    std::string jsonStr(reinterpret_cast<const char*>(data.Data()), data.Size());
    auto json = nlohmann::json::parse(jsonStr);
    return Deserialize(json, maxSize, maxItems);
}

nlohmann::json JsonSerializer::SerializeStackItem(std::shared_ptr<vm::StackItem> item, int64_t& itemCount)
{
    if (++itemCount > 2048)  // Default max items
        throw std::runtime_error("Maximum item count exceeded");

    if (!item)
        return nlohmann::json::value_t::null;

    // Basic serialization - just return type name for now
    // TODO: Implement proper serialization when VM API is stabilized
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

        default:
            return nlohmann::json::object({{"type", static_cast<int>(item->GetType())}});
    }
}

std::shared_ptr<vm::StackItem> JsonSerializer::DeserializeStackItem(const nlohmann::json& json, int64_t maxSize,
                                                                    int64_t maxItems, int64_t& itemCount)
{
    if (++itemCount > maxItems)
        throw std::runtime_error("Maximum item count exceeded");

    // Basic deserialization - return null for now
    // TODO: Implement proper deserialization when VM API is stabilized
    return vm::StackItem::Null();
}
}  // namespace neo::smartcontract