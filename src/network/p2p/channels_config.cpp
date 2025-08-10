#include <neo/network/ip_address.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/channels_config.h>

namespace neo::network::p2p
{
ChannelsConfig::ChannelsConfig()
    : tcp_(IPAddress::Any(), 10333),
      webSocket_(IPAddress::Any(), 10334),
      minDesiredConnections_(10),
      maxConnections_(20),
      maxConnectionsPerAddress_(3),
      maxKnownAddresses_(1000),
      maxKnownHashes_(1000)
{
}

const IPEndPoint& ChannelsConfig::GetTcp() const { return tcp_; }

void ChannelsConfig::SetTcp(const IPEndPoint& endpoint) { tcp_ = endpoint; }

const IPEndPoint& ChannelsConfig::GetWebSocket() const { return webSocket_; }

void ChannelsConfig::SetWebSocket(const IPEndPoint& endpoint) { webSocket_ = endpoint; }

uint32_t ChannelsConfig::GetMinDesiredConnections() const { return minDesiredConnections_; }

void ChannelsConfig::SetMinDesiredConnections(uint32_t minDesiredConnections)
{
    minDesiredConnections_ = minDesiredConnections;
}

uint32_t ChannelsConfig::GetMaxConnections() const { return maxConnections_; }

void ChannelsConfig::SetMaxConnections(uint32_t maxConnections) { maxConnections_ = maxConnections; }

uint32_t ChannelsConfig::GetMaxConnectionsPerAddress() const { return maxConnectionsPerAddress_; }

void ChannelsConfig::SetMaxConnectionsPerAddress(uint32_t maxConnectionsPerAddress)
{
    maxConnectionsPerAddress_ = maxConnectionsPerAddress;
}

uint32_t ChannelsConfig::GetMaxKnownAddresses() const { return maxKnownAddresses_; }

void ChannelsConfig::SetMaxKnownAddresses(uint32_t maxKnownAddresses) { maxKnownAddresses_ = maxKnownAddresses; }

uint32_t ChannelsConfig::GetMaxKnownHashes() const { return maxKnownHashes_; }

void ChannelsConfig::SetMaxKnownHashes(uint32_t maxKnownHashes) { maxKnownHashes_ = maxKnownHashes; }

const std::vector<IPEndPoint>& ChannelsConfig::GetSeedList() const { return seedList_; }

void ChannelsConfig::SetSeedList(const std::vector<IPEndPoint>& seedList) { seedList_ = seedList; }
}  // namespace neo::network::p2p
