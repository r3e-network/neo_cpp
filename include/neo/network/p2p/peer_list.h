#pragma once

#include <neo/network/p2p/peer.h>
#include <neo/network/ip_endpoint.h>
#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace neo::network::p2p
{
    /**
     * @brief Manages a list of peers.
     */
    class PeerList : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty PeerList.
         */
        PeerList();
        
        /**
         * @brief Gets the peers.
         * @return The peers.
         */
        std::vector<Peer> GetPeers() const;
        
        /**
         * @brief Gets the connected peers.
         * @return The connected peers.
         */
        std::vector<Peer> GetConnectedPeers() const;
        
        /**
         * @brief Gets the unconnected peers.
         * @return The unconnected peers.
         */
        std::vector<Peer> GetUnconnectedPeers() const;
        
        /**
         * @brief Gets the good peers.
         * @return The good peers.
         */
        std::vector<Peer> GetGoodPeers() const;
        
        /**
         * @brief Gets the bad peers.
         * @return The bad peers.
         */
        std::vector<Peer> GetBadPeers() const;
        
        /**
         * @brief Gets a peer by endpoint.
         * @param endpoint The endpoint.
         * @return The peer, or nullptr if not found.
         */
        Peer* GetPeer(const IPEndPoint& endpoint);
        
        /**
         * @brief Gets a peer by endpoint.
         * @param endpoint The endpoint.
         * @return The peer, or nullptr if not found.
         */
        const Peer* GetPeer(const IPEndPoint& endpoint) const;
        
        /**
         * @brief Adds a peer.
         * @param peer The peer.
         * @return True if the peer was added, false if it already exists.
         */
        bool AddPeer(const Peer& peer);
        
        /**
         * @brief Updates a peer.
         * @param peer The peer.
         * @return True if the peer was updated, false if it doesn't exist.
         */
        bool UpdatePeer(const Peer& peer);
        
        /**
         * @brief Removes a peer.
         * @param endpoint The endpoint of the peer.
         * @return True if the peer was removed, false if it doesn't exist.
         */
        bool RemovePeer(const IPEndPoint& endpoint);
        
        /**
         * @brief Clears all peers.
         */
        void Clear();
        
        /**
         * @brief Gets the number of peers.
         * @return The number of peers.
         */
        size_t GetCount() const;
        
        /**
         * @brief Gets the number of connected peers.
         * @return The number of connected peers.
         */
        size_t GetConnectedCount() const;
        
        /**
         * @brief Gets the number of unconnected peers.
         * @return The number of unconnected peers.
         */
        size_t GetUnconnectedCount() const;
        
        /**
         * @brief Gets the number of good peers.
         * @return The number of good peers.
         */
        size_t GetGoodCount() const;
        
        /**
         * @brief Gets the number of bad peers.
         * @return The number of bad peers.
         */
        size_t GetBadCount() const;
        
        /**
         * @brief Serializes the PeerList to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;
        
        /**
         * @brief Deserializes the PeerList from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;
        
        /**
         * @brief Serializes the PeerList to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;
        
        /**
         * @brief Deserializes the PeerList from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;
        
        /**
         * @brief Saves the peer list to a file.
         * @param path The path to the file.
         * @return True if the peer list was saved, false otherwise.
         */
        bool Save(const std::string& path) const;
        
        /**
         * @brief Loads the peer list from a file.
         * @param path The path to the file.
         * @return True if the peer list was loaded, false otherwise.
         */
        bool Load(const std::string& path);
        
    private:
        std::unordered_map<std::string, Peer> peers_;
        mutable std::mutex mutex_;
        
        std::string GetKey(const IPEndPoint& endpoint) const;
    };
}
