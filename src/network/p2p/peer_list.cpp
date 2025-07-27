#include <algorithm>
#include <chrono>
#include <fstream>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/peer.h>
#include <neo/network/p2p/peer_list.h>

namespace neo::network::p2p
{
PeerList::PeerList() = default;

std::vector<Peer> PeerList::GetPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Peer> peers;
    peers.reserve(peers_.size());

    for (const auto& pair : peers_)
    {
        peers.push_back(pair.second);
    }

    return peers;
}

std::vector<Peer> PeerList::GetConnectedPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Peer> peers;

    for (const auto& pair : peers_)
    {
        if (pair.second.IsConnected())
        {
            peers.push_back(pair.second);
        }
    }

    return peers;
}

std::vector<Peer> PeerList::GetUnconnectedPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Peer> peers;

    for (const auto& pair : peers_)
    {
        if (!pair.second.IsConnected() && !pair.second.IsBad())
        {
            peers.push_back(pair.second);
        }
    }

    return peers;
}

std::vector<Peer> PeerList::GetGoodPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Peer> peers;

    for (const auto& pair : peers_)
    {
        if (!pair.second.IsBad())
        {
            peers.push_back(pair.second);
        }
    }

    return peers;
}

std::vector<Peer> PeerList::GetBadPeers() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<Peer> peers;

    for (const auto& pair : peers_)
    {
        if (pair.second.IsBad())
        {
            peers.push_back(pair.second);
        }
    }

    return peers;
}

Peer* PeerList::GetPeer(const IPEndPoint& endpoint)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = GetKey(endpoint);
    auto it = peers_.find(key);

    if (it != peers_.end())
    {
        return &it->second;
    }

    return nullptr;
}

const Peer* PeerList::GetPeer(const IPEndPoint& endpoint) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = GetKey(endpoint);
    auto it = peers_.find(key);

    if (it != peers_.end())
    {
        return &it->second;
    }

    return nullptr;
}

bool PeerList::AddPeer(const Peer& peer)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = GetKey(peer.GetEndPoint());

    if (peers_.find(key) != peers_.end())
    {
        return false;
    }

    peers_[key] = peer;
    return true;
}

bool PeerList::UpdatePeer(const Peer& peer)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = GetKey(peer.GetEndPoint());
    auto it = peers_.find(key);

    if (it == peers_.end())
    {
        return false;
    }

    it->second = peer;
    return true;
}

bool PeerList::RemovePeer(const IPEndPoint& endpoint)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::string key = GetKey(endpoint);
    return peers_.erase(key) > 0;
}

void PeerList::Clear()
{
    std::lock_guard<std::mutex> lock(mutex_);
    peers_.clear();
}

size_t PeerList::GetCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return peers_.size();
}

size_t PeerList::GetConnectedCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;

    for (const auto& pair : peers_)
    {
        if (pair.second.IsConnected())
        {
            count++;
        }
    }

    return count;
}

size_t PeerList::GetUnconnectedCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;

    for (const auto& pair : peers_)
    {
        if (!pair.second.IsConnected() && !pair.second.IsBad())
        {
            count++;
        }
    }

    return count;
}

size_t PeerList::GetGoodCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;

    for (const auto& pair : peers_)
    {
        if (!pair.second.IsBad())
        {
            count++;
        }
    }

    return count;
}

size_t PeerList::GetBadCount() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = 0;

    for (const auto& pair : peers_)
    {
        if (pair.second.IsBad())
        {
            count++;
        }
    }

    return count;
}

void PeerList::Serialize(io::BinaryWriter& writer) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    writer.WriteVarInt(peers_.size());

    for (const auto& pair : peers_)
    {
        pair.second.Serialize(writer);
    }
}

void PeerList::Deserialize(io::BinaryReader& reader)
{
    std::lock_guard<std::mutex> lock(mutex_);

    peers_.clear();

    uint64_t count = reader.ReadVarInt();

    for (uint64_t i = 0; i < count; i++)
    {
        Peer peer;
        peer.Deserialize(reader);

        std::string key = GetKey(peer.GetEndPoint());
        peers_[key] = peer;
    }
}

void PeerList::SerializeJson(io::JsonWriter& writer) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json peersArray = nlohmann::json::array();

    for (const auto& pair : peers_)
    {
        nlohmann::json peerJson = nlohmann::json::object();
        io::JsonWriter peerWriter(peerJson);
        pair.second.SerializeJson(peerWriter);
        peersArray.push_back(peerJson);
    }

    writer.Write("peers", peersArray);
}

void PeerList::DeserializeJson(const io::JsonReader& reader)
{
    std::lock_guard<std::mutex> lock(mutex_);

    peers_.clear();

    auto peersArray = reader.ReadArray("peers");

    for (const auto& peerJson : peersArray)
    {
        Peer peer;
        io::JsonReader peerReader(peerJson);
        peer.DeserializeJson(peerReader);

        std::string key = GetKey(peer.GetEndPoint());
        peers_[key] = peer;
    }
}

bool PeerList::Save(const std::string& path) const
{
    try
    {
        std::ofstream file(path, std::ios::binary);

        if (!file)
        {
            return false;
        }

        io::BinaryWriter writer(file);
        Serialize(writer);

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool PeerList::Load(const std::string& path)
{
    try
    {
        std::ifstream file(path, std::ios::binary);

        if (!file)
        {
            return false;
        }

        io::BinaryReader reader(file);
        Deserialize(reader);

        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::string PeerList::GetKey(const IPEndPoint& endpoint) const
{
    return endpoint.GetAddress().ToString() + ":" + std::to_string(endpoint.GetPort());
}
}  // namespace neo::network::p2p
