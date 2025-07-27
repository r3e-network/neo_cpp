#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace neo::core
{

/**
 * @brief Network configuration constants and utilities
 */
class NetworkConfig
{
  public:
    struct NetworkSettings
    {
        uint32_t magic;
        uint16_t default_p2p_port;
        uint16_t default_rpc_port;
        uint16_t default_ws_port;
        uint32_t milliseconds_per_block;
        uint32_t max_transactions_per_block;
        uint32_t validators_count;
        uint32_t committee_members_count;
        std::string address_version;
    };

    /**
     * @brief Get network settings by name
     * @param network Network name (mainnet, testnet, privnet)
     * @return Network settings
     * @throws std::invalid_argument if network not found
     */
    static const NetworkSettings& GetNetworkSettings(const std::string& network)
    {
        auto it = network_settings_.find(network);
        if (it == network_settings_.end())
        {
            throw std::invalid_argument("Unknown network: " + network);
        }
        return it->second;
    }

    /**
     * @brief Get network magic number by name
     * @param network Network name
     * @return Magic number
     */
    static uint32_t GetNetworkMagic(const std::string& network)
    {
        return GetNetworkSettings(network).magic;
    }

    /**
     * @brief Get default port for service on network
     * @param network Network name
     * @param service Service type (p2p, rpc, ws)
     * @return Default port
     */
    static uint16_t GetDefaultPort(const std::string& network, const std::string& service)
    {
        const auto& settings = GetNetworkSettings(network);

        if (service == "p2p")
        {
            return settings.default_p2p_port;
        }
        else if (service == "rpc")
        {
            return settings.default_rpc_port;
        }
        else if (service == "ws")
        {
            return settings.default_ws_port;
        }
        else
        {
            throw std::invalid_argument("Unknown service type: " + service);
        }
    }

    /**
     * @brief Check if network name is valid
     * @param network Network name to check
     * @return true if valid
     */
    static bool IsValidNetwork(const std::string& network)
    {
        return network_settings_.find(network) != network_settings_.end();
    }

    /**
     * @brief Get all available network names
     * @return Vector of network names
     */
    static std::vector<std::string> GetAvailableNetworks()
    {
        std::vector<std::string> networks;
        for (const auto& pair : network_settings_)
        {
            networks.push_back(pair.first);
        }
        return networks;
    }

  private:
    static const std::unordered_map<std::string, NetworkSettings> network_settings_;
};

// Network configuration definitions
const std::unordered_map<std::string, NetworkConfig::NetworkSettings> NetworkConfig::network_settings_ = {
    {"mainnet",
     {.magic = 0x3346454E,  // 860833102 in decimal
      .default_p2p_port = 10333,
      .default_rpc_port = 10332,
      .default_ws_port = 10334,
      .milliseconds_per_block = 15000,
      .max_transactions_per_block = 512,
      .validators_count = 7,
      .committee_members_count = 21,
      .address_version = "N"}},
    {"testnet",
     {.magic = 0x3554334E,  // 894710606 in decimal
      .default_p2p_port = 20333,
      .default_rpc_port = 20332,
      .default_ws_port = 20334,
      .milliseconds_per_block = 15000,
      .max_transactions_per_block = 512,
      .validators_count = 7,
      .committee_members_count = 21,
      .address_version = "N"}},
    {"privnet",
     {.magic = 0x746E4152,  // 1953787474 in decimal
      .default_p2p_port = 30333,
      .default_rpc_port = 30332,
      .default_ws_port = 30334,
      .milliseconds_per_block = 15000,
      .max_transactions_per_block = 512,
      .validators_count = 4,
      .committee_members_count = 4,
      .address_version = "N"}}};

}  // namespace neo::core