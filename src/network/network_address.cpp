#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/network_address.h>

namespace neo::network
{
NetworkAddress::NetworkAddress() : timestamp_(0), services_(0), port_(0) {}

NetworkAddress::NetworkAddress(uint32_t timestamp, uint64_t services, const std::string& address, uint16_t port)
    : timestamp_(timestamp), services_(services), address_(address), port_(port)
{
}

uint32_t NetworkAddress::GetTimestamp() const { return timestamp_; }

uint64_t NetworkAddress::GetServices() const { return services_; }

const std::string& NetworkAddress::GetAddress() const { return address_; }

uint16_t NetworkAddress::GetPort() const { return port_; }

void NetworkAddress::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(timestamp_);
    writer.Write(services_);
    writer.WriteString(address_);
    writer.Write(port_);
}

void NetworkAddress::Deserialize(io::BinaryReader& reader)
{
    timestamp_ = reader.ReadUInt32();
    services_ = reader.ReadUInt64();
    address_ = reader.ReadString();
    port_ = reader.ReadUInt16();
}

void NetworkAddress::SerializeJson(io::JsonWriter& writer) const
{
    writer.Write("timestamp", timestamp_);
    writer.Write("services", services_);
    writer.Write("address", address_);
    writer.Write("port", port_);
}

void NetworkAddress::DeserializeJson(const io::JsonReader& reader)
{
    timestamp_ = reader.ReadUInt32("timestamp");
    services_ = reader.ReadUInt64("services");
    address_ = reader.ReadString("address");
    port_ = reader.ReadUInt16("port");
}
}  // namespace neo::network
