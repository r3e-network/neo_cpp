#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <neo/hardfork.h>
#include <neo/cryptography/ecc/ecpoint.h>

namespace neo
{
    /**
     * @brief Represents the protocol settings of the NEO system.
     * 
     * This class contains all configuration parameters that define the behavior
     * of the Neo blockchain protocol, including network settings, consensus
     * parameters, transaction limits, and hardfork configurations.
     */
    class ProtocolSettings
    {
    public:
        /**
         * @brief Constructs default ProtocolSettings.
         */
        ProtocolSettings();

        /**
         * @brief Copy constructor
         */
        ProtocolSettings(const ProtocolSettings& other);

        /**
         * @brief Assignment operator
         */
        ProtocolSettings& operator=(const ProtocolSettings& other);

        /**
         * @brief Gets the network magic number.
         * @return The network magic number.
         */
        uint32_t GetNetwork() const;

        /**
         * @brief Sets the network magic number.
         * @param network The network magic number.
         */
        void SetNetwork(uint32_t network);

        /**
         * @brief Gets the address version.
         * @return The address version byte.
         */
        uint8_t GetAddressVersion() const;

        /**
         * @brief Sets the address version.
         * @param addressVersion The address version byte.
         */
        void SetAddressVersion(uint8_t addressVersion);

        /**
         * @brief Gets the standby committee members.
         * @return Vector of ECPoint representing committee members.
         */
        const std::vector<neo::cryptography::ecc::ECPoint>& GetStandbyCommittee() const;

        /**
         * @brief Sets the standby committee members.
         * @param committee Vector of ECPoint representing committee members.
         */
        void SetStandbyCommittee(const std::vector<neo::cryptography::ecc::ECPoint>& committee);

        /**
         * @brief Gets the number of committee members.
         * @return The committee members count.
         */
        int GetCommitteeMembersCount() const;

        /**
         * @brief Gets the number of validators.
         * @return The validators count.
         */
        int GetValidatorsCount() const;

        /**
         * @brief Sets the number of validators.
         * @param validatorsCount The validators count.
         */
        void SetValidatorsCount(int validatorsCount);

        /**
         * @brief Gets the standby validators (first N committee members).
         * @return Vector of ECPoint representing validators.
         */
        std::vector<neo::cryptography::ecc::ECPoint> GetStandbyValidators() const;

        /**
         * @brief Gets the seed list.
         * @return Vector of seed node addresses.
         */
        const std::vector<std::string>& GetSeedList() const;

        /**
         * @brief Sets the seed list.
         * @param seedList Vector of seed node addresses.
         */
        void SetSeedList(const std::vector<std::string>& seedList);

        /**
         * @brief Gets the time in milliseconds between two blocks.
         * @return The milliseconds per block.
         */
        uint32_t GetMillisecondsPerBlock() const;

        /**
         * @brief Sets the time in milliseconds between two blocks.
         * @param millisecondsPerBlock The milliseconds per block.
         */
        void SetMillisecondsPerBlock(uint32_t millisecondsPerBlock);

        /**
         * @brief Gets the maximum increment of ValidUntilBlock field.
         * @return The maximum valid until block increment.
         */
        uint32_t GetMaxValidUntilBlockIncrement() const;

        /**
         * @brief Sets the maximum increment of ValidUntilBlock field.
         * @param maxValidUntilBlockIncrement The maximum valid until block increment.
         */
        void SetMaxValidUntilBlockIncrement(uint32_t maxValidUntilBlockIncrement);

        /**
         * @brief Gets the maximum number of transactions per block.
         * @return The maximum transactions per block.
         */
        uint32_t GetMaxTransactionsPerBlock() const;

        /**
         * @brief Sets the maximum number of transactions per block.
         * @param maxTransactionsPerBlock The maximum transactions per block.
         */
        void SetMaxTransactionsPerBlock(uint32_t maxTransactionsPerBlock);

        /**
         * @brief Gets the maximum number of transactions in memory pool.
         * @return The memory pool max transactions.
         */
        int GetMemoryPoolMaxTransactions() const;

        /**
         * @brief Sets the maximum number of transactions in memory pool.
         * @param memoryPoolMaxTransactions The memory pool max transactions.
         */
        void SetMemoryPoolMaxTransactions(int memoryPoolMaxTransactions);

        /**
         * @brief Gets the maximum number of traceable blocks.
         * @return The maximum number of traceable blocks.
         */
        uint32_t GetMaxTraceableBlocks() const;

        /**
         * @brief Sets the maximum number of traceable blocks.
         * @param maxTraceableBlocks The maximum number of traceable blocks.
         */
        void SetMaxTraceableBlocks(uint32_t maxTraceableBlocks);

        /**
         * @brief Gets the initial gas distribution amount.
         * @return The initial gas distribution in datoshi.
         */
        uint64_t GetInitialGasDistribution() const;

        /**
         * @brief Sets the initial gas distribution amount.
         * @param initialGasDistribution The initial gas distribution in datoshi.
         */
        void SetInitialGasDistribution(uint64_t initialGasDistribution);

        /**
         * @brief Gets the hardfork configuration.
         * @return Map of hardfork to activation block height.
         */
        const std::unordered_map<Hardfork, uint32_t>& GetHardforks() const;

        /**
         * @brief Sets the hardfork configuration.
         * @param hardforks Map of hardfork to activation block height.
         */
        void SetHardforks(const std::unordered_map<Hardfork, uint32_t>& hardforks);

        /**
         * @brief Check if a hardfork is enabled at a given block height.
         * @param hardfork The hardfork to check.
         * @param blockHeight The block height to check against.
         * @return True if the hardfork is enabled at the given height.
         */
        bool IsHardforkEnabled(Hardfork hardfork, uint32_t blockHeight) const;

        /**
         * @brief Loads protocol settings from a JSON file.
         * @param filePath Path to the JSON configuration file.
         * @return Loaded ProtocolSettings instance.
         */
        static std::unique_ptr<ProtocolSettings> Load(const std::string& filePath);

        /**
         * @brief Loads protocol settings from a JSON string.
         * @param jsonContent JSON configuration content.
         * @return Loaded ProtocolSettings instance.
         */
        static std::unique_ptr<ProtocolSettings> LoadFromJson(const std::string& jsonContent);

        /**
         * @brief Gets the default protocol settings.
         * @return Default ProtocolSettings instance.
         */
        static const ProtocolSettings& GetDefault();

        /**
         * @brief Validates the hardfork configuration for consistency.
         * @throws std::invalid_argument if hardfork configuration is invalid.
         */
        void ValidateHardforkConfiguration() const;

        /**
         * @brief Ensures all hardforks have entries in the configuration.
         * @param hardforks The hardfork map to process.
         * @return Processed hardfork map with all entries.
         */
        static std::unordered_map<Hardfork, uint32_t> EnsureOmittedHardforks(
            const std::unordered_map<Hardfork, uint32_t>& hardforks);

    private:
        uint32_t network_;
        uint8_t addressVersion_;
        std::vector<neo::cryptography::ecc::ECPoint> standbyCommittee_;
        int validatorsCount_;
        std::vector<std::string> seedList_;
        uint32_t millisecondsPerBlock_;
        uint32_t maxValidUntilBlockIncrement_;
        uint32_t maxTransactionsPerBlock_;
        int memoryPoolMaxTransactions_;
        uint32_t maxTraceableBlocks_;
        uint64_t initialGasDistribution_;
        std::unordered_map<Hardfork, uint32_t> hardforks_;

        /**
         * @brief Validates hardfork configuration for continuity and ordering.
         * @param settings The settings to validate.
         */
        static void CheckHardforkConfiguration(const ProtocolSettings& settings);
    };
}
