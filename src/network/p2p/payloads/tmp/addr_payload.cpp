#include <limits>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/payloads/addr_payload.h>
#include <stdexcept>

namespace neo::network::payloads
{
AddrPayload::AddrPayload() = default;

AddrPayload::AddrPayload(const std::vector<NetworkAddress>& addresses) : addresses_(addresses) {}

const std::vector<NetworkAddress>& AddrPayload::GetAddresses() const
{
    return addresses_;
}

void AddrPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarInt(addresses_.size());
    for (const auto& address : addresses_)
    {
        address.Serialize(writer);
    }
}

void AddrPayload::Deserialize(io::BinaryReader& reader)
{
    int64_t count = reader.ReadVarInt();
    if (count < 0 || count > std::numeric_limits<size_t>::max())
        throw std::out_of_range("Invalid address count");

    addresses_.clear();
    addresses_.reserve(static_cast<size_t>(count));

    for (int64_t i = 0; i < count; i++)
    {
        NetworkAddress address;
        address.Deserialize(reader);
        addresses_.push_back(address);
    }
}

void AddrPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartArray("addresses");
    for (const auto& address : addresses_)
    {
        writer.WriteStartObject();
        address.SerializeJson(writer);
        writer.WriteEndObject();
    }
    writer.WriteEndArray();
}

void AddrPayload::DeserializeJson(const io::JsonReader& reader)
{
    auto addressesArray = reader.ReadArray("addresses");
    addresses_.clear();
    addresses_.reserve(addressesArray.size());

    for (const auto& addressJson : addressesArray)
    {
        NetworkAddress address;
        io::JsonReader addressReader(addressJson);
        address.DeserializeJson(addressReader);
        addresses_.push_back(address);
    }
}
}  // namespace neo::network::payloads
