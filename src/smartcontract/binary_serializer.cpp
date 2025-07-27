#include <neo/smartcontract/binary_serializer.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/stack_item_types.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract
{
io::ByteVector BinarySerializer::Serialize(std::shared_ptr<vm::StackItem> item, int64_t maxSize, int64_t maxItems)
{
    // Basic serialization implementation
    std::ostringstream stream;
    io::BinaryWriter writer(stream);

    // TODO: Implement proper serialization when VM API is stabilized
    writer.WriteByte(static_cast<uint8_t>(item->GetType()));

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

    // TODO: Implement proper deserialization when VM API is stabilized
    auto type = static_cast<vm::StackItemType>(reader.ReadByte());

    // Return null item for now
    return vm::StackItem::Null();
}

void BinarySerializer::SerializeStackItem(io::BinaryWriter& writer, std::shared_ptr<vm::StackItem> item,
                                          int64_t maxSize, int64_t maxItems, int64_t& itemCount)
{
    if (++itemCount > maxItems)
        throw std::runtime_error("Maximum item count exceeded");

    writer.WriteByte(static_cast<uint8_t>(item->GetType()));

    // TODO: Implement proper serialization for each type
}

std::shared_ptr<vm::StackItem> BinarySerializer::DeserializeStackItem(io::BinaryReader& reader, int64_t maxSize,
                                                                      int64_t maxItems, int64_t& itemCount)
{
    if (++itemCount > maxItems)
        throw std::runtime_error("Maximum item count exceeded");

    auto type = static_cast<vm::StackItemType>(reader.ReadByte());

    // TODO: Implement proper deserialization for each type
    return vm::StackItem::Null();
}
}  // namespace neo::smartcontract