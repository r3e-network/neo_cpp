#pragma once

#include <neo/network/ip_endpoint.h>
#include <cstdint>
#include <vector>

namespace neo::network::p2p
{
    /**
     * @brief Configuration for P2P channels.
     */
    class ChannelsConfig
    {
    public:
        /**
         * @brief Constructs a ChannelsConfig.
         */
        ChannelsConfig();

        /**
         * @brief Gets the TCP endpoint.
         * @return The TCP endpoint.
         */
        const IPEndPoint& GetTcp() const;

        /**
         * @brief Sets the TCP endpoint.
         * @param endpoint The TCP endpoint.
         */
        void SetTcp(const IPEndPoint& endpoint);

        /**
         * @brief Gets the WebSocket endpoint.
         * @return The WebSocket endpoint.
         */
        const IPEndPoint& GetWebSocket() const;

        /**
         * @brief Sets the WebSocket endpoint.
         * @param endpoint The WebSocket endpoint.
         */
        void SetWebSocket(const IPEndPoint& endpoint);

        /**
         * @brief Gets the minimum desired connections.
         * @return The minimum desired connections.
         */
        uint32_t GetMinDesiredConnections() const;

        /**
         * @brief Sets the minimum desired connections.
         * @param minDesiredConnections The minimum desired connections.
         */
        void SetMinDesiredConnections(uint32_t minDesiredConnections);

        /**
         * @brief Gets the maximum connections.
         * @return The maximum connections.
         */
        uint32_t GetMaxConnections() const;

        /**
         * @brief Sets the maximum connections.
         * @param maxConnections The maximum connections.
         */
        void SetMaxConnections(uint32_t maxConnections);

        /**
         * @brief Gets the maximum connections per address.
         * @return The maximum connections per address.
         */
        uint32_t GetMaxConnectionsPerAddress() const;

        /**
         * @brief Sets the maximum connections per address.
         * @param maxConnectionsPerAddress The maximum connections per address.
         */
        void SetMaxConnectionsPerAddress(uint32_t maxConnectionsPerAddress);

        /**
         * @brief Gets the maximum known addresses.
         * @return The maximum known addresses.
         */
        uint32_t GetMaxKnownAddresses() const;

        /**
         * @brief Sets the maximum known addresses.
         * @param maxKnownAddresses The maximum known addresses.
         */
        void SetMaxKnownAddresses(uint32_t maxKnownAddresses);

        /**
         * @brief Gets the maximum known hashes.
         * @return The maximum known hashes.
         */
        uint32_t GetMaxKnownHashes() const;

        /**
         * @brief Sets the maximum known hashes.
         * @param maxKnownHashes The maximum known hashes.
         */
        void SetMaxKnownHashes(uint32_t maxKnownHashes);

        /**
         * @brief Gets the seed list.
         * @return The seed list.
         */
        const std::vector<IPEndPoint>& GetSeedList() const;

        /**
         * @brief Sets the seed list.
         * @param seedList The seed list.
         */
        void SetSeedList(const std::vector<IPEndPoint>& seedList);

    private:
        IPEndPoint tcp_;
        IPEndPoint webSocket_;
        uint32_t minDesiredConnections_;
        uint32_t maxConnections_;
        uint32_t maxConnectionsPerAddress_;
        uint32_t maxKnownAddresses_;
        uint32_t maxKnownHashes_;
        std::vector<IPEndPoint> seedList_;
    };
}
