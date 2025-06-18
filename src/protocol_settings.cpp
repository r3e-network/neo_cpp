#include <neo/protocol_settings.h>
#include <neo/hardfork.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <filesystem>

using json = nlohmann::json;

namespace neo
{
    // Default values matching C# implementation
    namespace defaults
    {
        constexpr uint32_t NETWORK = 0x334F454E; // MainNet magic
        constexpr uint8_t ADDRESS_VERSION = 0x35;
        constexpr uint32_t MILLISECONDS_PER_BLOCK = 15000;
        constexpr uint32_t MAX_TRANSACTIONS_PER_BLOCK = 512;
        constexpr uint32_t MAX_VALID_UNTIL_BLOCK_INCREMENT = 5760; // 86400000 / 15000
        constexpr int MEMORY_POOL_MAX_TRANSACTIONS = 50000;
        constexpr uint32_t MAX_TRACEABLE_BLOCKS = 2102400;
        constexpr uint64_t INITIAL_GAS_DISTRIBUTION = 5200000000000000ull;
    }

    ProtocolSettings::ProtocolSettings()
        : network_(defaults::NETWORK)
        , addressVersion_(defaults::ADDRESS_VERSION)
        , standbyCommittee_()
        , validatorsCount_(0)
        , seedList_()
        , millisecondsPerBlock_(defaults::MILLISECONDS_PER_BLOCK)
        , maxValidUntilBlockIncrement_(defaults::MAX_VALID_UNTIL_BLOCK_INCREMENT)
        , maxTransactionsPerBlock_(defaults::MAX_TRANSACTIONS_PER_BLOCK)
        , memoryPoolMaxTransactions_(defaults::MEMORY_POOL_MAX_TRANSACTIONS)
        , maxTraceableBlocks_(defaults::MAX_TRACEABLE_BLOCKS)
        , initialGasDistribution_(defaults::INITIAL_GAS_DISTRIBUTION)
        , hardforks_(EnsureOmittedHardforks({}))
    {
    }

    ProtocolSettings::ProtocolSettings(const ProtocolSettings& other)
        : network_(other.network_)
        , addressVersion_(other.addressVersion_)
        , standbyCommittee_(other.standbyCommittee_)
        , validatorsCount_(other.validatorsCount_)
        , seedList_(other.seedList_)
        , millisecondsPerBlock_(other.millisecondsPerBlock_)
        , maxValidUntilBlockIncrement_(other.maxValidUntilBlockIncrement_)
        , maxTransactionsPerBlock_(other.maxTransactionsPerBlock_)
        , memoryPoolMaxTransactions_(other.memoryPoolMaxTransactions_)
        , maxTraceableBlocks_(other.maxTraceableBlocks_)
        , initialGasDistribution_(other.initialGasDistribution_)
        , hardforks_(other.hardforks_)
    {
    }

    ProtocolSettings& ProtocolSettings::operator=(const ProtocolSettings& other)
    {
        if (this != &other)
        {
            network_ = other.network_;
            addressVersion_ = other.addressVersion_;
            standbyCommittee_ = other.standbyCommittee_;
            validatorsCount_ = other.validatorsCount_;
            seedList_ = other.seedList_;
            millisecondsPerBlock_ = other.millisecondsPerBlock_;
            maxValidUntilBlockIncrement_ = other.maxValidUntilBlockIncrement_;
            maxTransactionsPerBlock_ = other.maxTransactionsPerBlock_;
            memoryPoolMaxTransactions_ = other.memoryPoolMaxTransactions_;
            maxTraceableBlocks_ = other.maxTraceableBlocks_;
            initialGasDistribution_ = other.initialGasDistribution_;
            hardforks_ = other.hardforks_;
        }
        return *this;
    }

    // Getters
    uint32_t ProtocolSettings::GetNetwork() const { return network_; }
    uint8_t ProtocolSettings::GetAddressVersion() const { return addressVersion_; }
    const std::vector<neo::cryptography::ecc::ECPoint>& ProtocolSettings::GetStandbyCommittee() const { return standbyCommittee_; }
    int ProtocolSettings::GetCommitteeMembersCount() const { return static_cast<int>(standbyCommittee_.size()); }
    int ProtocolSettings::GetValidatorsCount() const { return validatorsCount_; }
    const std::vector<std::string>& ProtocolSettings::GetSeedList() const { return seedList_; }
    uint32_t ProtocolSettings::GetMillisecondsPerBlock() const { return millisecondsPerBlock_; }
    uint32_t ProtocolSettings::GetMaxValidUntilBlockIncrement() const { return maxValidUntilBlockIncrement_; }
    uint32_t ProtocolSettings::GetMaxTransactionsPerBlock() const { return maxTransactionsPerBlock_; }
    int ProtocolSettings::GetMemoryPoolMaxTransactions() const { return memoryPoolMaxTransactions_; }
    uint32_t ProtocolSettings::GetMaxTraceableBlocks() const { return maxTraceableBlocks_; }
    uint64_t ProtocolSettings::GetInitialGasDistribution() const { return initialGasDistribution_; }
    const std::unordered_map<Hardfork, uint32_t>& ProtocolSettings::GetHardforks() const { return hardforks_; }

    // Setters
    void ProtocolSettings::SetNetwork(uint32_t network) { network_ = network; }
    void ProtocolSettings::SetAddressVersion(uint8_t addressVersion) { addressVersion_ = addressVersion; }
    void ProtocolSettings::SetStandbyCommittee(const std::vector<neo::cryptography::ecc::ECPoint>& committee) { standbyCommittee_ = committee; }
    void ProtocolSettings::SetValidatorsCount(int validatorsCount) { validatorsCount_ = validatorsCount; }
    void ProtocolSettings::SetSeedList(const std::vector<std::string>& seedList) { seedList_ = seedList; }
    void ProtocolSettings::SetMillisecondsPerBlock(uint32_t millisecondsPerBlock) { millisecondsPerBlock_ = millisecondsPerBlock; }
    void ProtocolSettings::SetMaxValidUntilBlockIncrement(uint32_t maxValidUntilBlockIncrement) { maxValidUntilBlockIncrement_ = maxValidUntilBlockIncrement; }
    void ProtocolSettings::SetMaxTransactionsPerBlock(uint32_t maxTransactionsPerBlock) { maxTransactionsPerBlock_ = maxTransactionsPerBlock; }
    void ProtocolSettings::SetMemoryPoolMaxTransactions(int memoryPoolMaxTransactions) { memoryPoolMaxTransactions_ = memoryPoolMaxTransactions; }
    void ProtocolSettings::SetMaxTraceableBlocks(uint32_t maxTraceableBlocks) { maxTraceableBlocks_ = maxTraceableBlocks; }
    void ProtocolSettings::SetInitialGasDistribution(uint64_t initialGasDistribution) { initialGasDistribution_ = initialGasDistribution; }
    void ProtocolSettings::SetHardforks(const std::unordered_map<Hardfork, uint32_t>& hardforks) { hardforks_ = hardforks; }

    std::vector<neo::cryptography::ecc::ECPoint> ProtocolSettings::GetStandbyValidators() const
    {
        std::vector<neo::cryptography::ecc::ECPoint> validators;
        int count = std::min(validatorsCount_, static_cast<int>(standbyCommittee_.size()));
        validators.reserve(count);
        
        for (int i = 0; i < count; ++i)
        {
            validators.push_back(standbyCommittee_[i]);
        }
        
        return validators;
    }

    bool ProtocolSettings::IsHardforkEnabled(Hardfork hardfork, uint32_t blockHeight) const
    {
        auto it = hardforks_.find(hardfork);
        if (it != hardforks_.end())
        {
            return blockHeight >= it->second;
        }
        return false;
    }

    std::unordered_map<Hardfork, uint32_t> ProtocolSettings::EnsureOmittedHardforks(
        const std::unordered_map<Hardfork, uint32_t>& hardforks)
    {
        std::unordered_map<Hardfork, uint32_t> result = hardforks;
        
        // Add all hardforks with default value 0 if not present
        for (int i = 0; i < GetHardforkCount(); ++i)
        {
            Hardfork hf = static_cast<Hardfork>(i);
            if (result.find(hf) == result.end())
            {
                result[hf] = 0;
            }
            else
            {
                // If we find a configured hardfork, stop adding defaults
                break;
            }
        }
        
        return result;
    }

    void ProtocolSettings::CheckHardforkConfiguration(const ProtocolSettings& settings)
    {
        const auto& hardforks = settings.GetHardforks();
        
        // Get all configured hardforks and sort by enum value
        std::vector<std::pair<Hardfork, uint32_t>> sortedHardforks;
        for (const auto& [hf, height] : hardforks)
        {
            sortedHardforks.emplace_back(hf, height);
        }
        
        std::sort(sortedHardforks.begin(), sortedHardforks.end(),
            [](const auto& a, const auto& b) {
                return static_cast<int>(a.first) < static_cast<int>(b.first);
            });
        
        // Check for continuity and ordering
        for (size_t i = 0; i < sortedHardforks.size() - 1; ++i)
        {
            int currentIndex = static_cast<int>(sortedHardforks[i].first);
            int nextIndex = static_cast<int>(sortedHardforks[i + 1].first);
            
            // Check continuity
            if (nextIndex - currentIndex > 1)
            {
                throw std::invalid_argument("Hardfork configuration is not continuous.");
            }
            
            // Check ordering of block heights
            if (sortedHardforks[i].second > sortedHardforks[i + 1].second)
            {
                throw std::invalid_argument(
                    "The Hardfork configuration for " + std::string(HardforkToString(sortedHardforks[i].first)) +
                    " is greater than for " + std::string(HardforkToString(sortedHardforks[i + 1].first)));
            }
        }
    }

    void ProtocolSettings::ValidateHardforkConfiguration() const
    {
        CheckHardforkConfiguration(*this);
    }

    const ProtocolSettings& ProtocolSettings::GetDefault()
    {
        static ProtocolSettings defaultSettings;
        return defaultSettings;
    }
}
