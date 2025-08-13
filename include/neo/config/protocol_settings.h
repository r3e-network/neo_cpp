/**
 * @file protocol_settings.h
 * @brief Configuration settings
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace neo::config
{
/**
 * @brief Protocol settings for the Neo blockchain
 */
struct ProtocolSettings
{
    uint32_t Network = 860833102;
    uint8_t AddressVersion = 53;
    uint32_t MillisecondsPerBlock = 15000;
    uint32_t MaxTransactionsPerBlock = 512;
    uint32_t MemoryPoolMaxTransactions = 50000;
    uint32_t MaxTraceableBlocks = 2102400;
    uint64_t InitialGasDistribution = 5200000000000000;
    uint32_t ValidatorsCount = 7;

    std::vector<std::string> StandbyCommittee;
    std::vector<std::string> SeedList;

    /**
     * @brief Get default protocol settings
     */
    static ProtocolSettings GetDefault()
    {
        ProtocolSettings settings;
        settings.Network = 860833102;
        settings.AddressVersion = 53;
        settings.MillisecondsPerBlock = 15000;
        settings.MaxTransactionsPerBlock = 512;
        settings.MemoryPoolMaxTransactions = 50000;
        settings.MaxTraceableBlocks = 2102400;
        settings.InitialGasDistribution = 5200000000000000;
        settings.ValidatorsCount = 7;

        // Default seed list
        settings.SeedList = {"seed1.neo.org:10333", "seed2.neo.org:10333", "seed3.neo.org:10333", "seed4.neo.org:10333",
                             "seed5.neo.org:10333"};

        return settings;
    }

    /**
     * @brief Get network magic number
     */
    uint32_t GetNetwork() const { return Network; }
};
}  // namespace neo::config