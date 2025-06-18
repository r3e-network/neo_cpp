#include <neo/network/p2p/network_address_with_time.h>

namespace neo::network::p2p
{
    NetworkAddressWithTime::NetworkAddressWithTime()
        : timestamp_(0), services_(0), port_(0)
    {
    }

    NetworkAddressWithTime::NetworkAddressWithTime(uint32_t timestamp, uint64_t services, const std::string& address, uint16_t port)
        : timestamp_(timestamp), services_(services), address_(address), port_(port)
    {
    }

    uint32_t NetworkAddressWithTime::GetTimestamp() const
    {
        return timestamp_;
    }

    void NetworkAddressWithTime::SetTimestamp(uint32_t timestamp)
    {
        timestamp_ = timestamp;
    }

    uint64_t NetworkAddressWithTime::GetServices() const
    {
        return services_;
    }

    void NetworkAddressWithTime::SetServices(uint64_t services)
    {
        services_ = services;
    }

    const std::string& NetworkAddressWithTime::GetAddress() const
    {
        return address_;
    }

    void NetworkAddressWithTime::SetAddress(const std::string& address)
    {
        address_ = address;
    }

    uint16_t NetworkAddressWithTime::GetPort() const
    {
        return port_;
    }

    void NetworkAddressWithTime::SetPort(uint16_t port)
    {
        port_ = port;
    }

    void NetworkAddressWithTime::Serialize(io::BinaryWriter& writer) const
    {
        writer.Write(timestamp_);
        writer.Write(services_);
        writer.WriteVarString(address_);
        writer.Write(port_);
    }

    void NetworkAddressWithTime::Deserialize(io::BinaryReader& reader)
    {
        timestamp_ = reader.ReadUInt32();
        services_ = reader.ReadUInt64();
        address_ = reader.ReadVarString();
        port_ = reader.ReadUInt16();
    }

    int NetworkAddressWithTime::GetSize() const
    {
        return sizeof(uint32_t) + sizeof(uint64_t) + static_cast<int>(1 + address_.length()) + sizeof(uint16_t);
    }

    void NetworkAddressWithTime::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();
        writer.Write("timestamp", timestamp_);
        writer.Write("services", services_);
        writer.Write("address", address_);
        writer.Write("port", port_);
        writer.WriteEndObject();
    }

    void NetworkAddressWithTime::DeserializeJson(const io::JsonReader& reader)
    {
        reader.ReadStartObject();
        timestamp_ = reader.ReadUInt32("timestamp");
        services_ = reader.ReadUInt64("services");
        address_ = reader.ReadString("address");
        port_ = reader.ReadUInt16("port");
        reader.ReadEndObject();
    }

    bool NetworkAddressWithTime::operator==(const NetworkAddressWithTime& other) const
    {
        return timestamp_ == other.timestamp_ &&
               services_ == other.services_ &&
               address_ == other.address_ &&
               port_ == other.port_;
    }

    bool NetworkAddressWithTime::operator!=(const NetworkAddressWithTime& other) const
    {
        return !(*this == other);
    }
}
