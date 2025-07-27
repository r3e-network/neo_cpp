#include <neo/network/p2p/payloads/addr_payload.h>
#include <stdexcept>

namespace neo::network::p2p::payloads
{
AddrPayload::AddrPayload() = default;

AddrPayload::AddrPayload(const std::vector<NetworkAddressWithTime>& addresses) : addressList_(addresses) {}

const std::vector<NetworkAddressWithTime>& AddrPayload::GetAddressList() const
{
    return addressList_;
}

void AddrPayload::SetAddressList(const std::vector<NetworkAddressWithTime>& addresses)
{
    addressList_ = addresses;
}

int AddrPayload::GetSize() const
{
    // Calculate the size based on addresses
    // This matches the C# implementation: AddressList.GetVarSize()
    int size = 0;

    // VarInt for count
    if (addressList_.size() < 0xFD)
        size += sizeof(uint8_t);
    else if (addressList_.size() <= 0xFFFF)
        size += sizeof(uint8_t) + sizeof(uint16_t);
    else
        size += sizeof(uint8_t) + sizeof(uint32_t);

    // Each address has its own size
    for (const auto& address : addressList_)
    {
        size += address.GetSize();
    }

    return size;
}

void AddrPayload::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarInt(addressList_.size());

    for (const auto& address : addressList_)
    {
        address.Serialize(writer);
    }
}

void AddrPayload::Deserialize(io::BinaryReader& reader)
{
    uint64_t count = reader.ReadVarInt(MaxCountToSend);
    addressList_.clear();
    addressList_.reserve(count);

    for (uint64_t i = 0; i < count; i++)
    {
        NetworkAddressWithTime address;
        address.Deserialize(reader);
        addressList_.push_back(address);
    }
}

void AddrPayload::SerializeJson(io::JsonWriter& writer) const
{
    nlohmann::json addressesArray = nlohmann::json::array();

    for (const auto& address : addressList_)
    {
        nlohmann::json addressJson = nlohmann::json::object();
        io::JsonWriter addressWriter(addressJson);
        address.SerializeJson(addressWriter);
        addressesArray.push_back(addressJson);
    }

    writer.Write("addressList", addressesArray);
}

void AddrPayload::DeserializeJson(const io::JsonReader& reader)
{
    auto addressesArray = reader.ReadArray("addressList");
    addressList_.clear();
    addressList_.reserve(addressesArray.size());

    for (const auto& addressJson : addressesArray)
    {
        NetworkAddressWithTime address;
        io::JsonReader addressReader(addressJson);
        address.DeserializeJson(addressReader);
        addressList_.push_back(address);
    }
}
}  // namespace neo::network::p2p::payloads
