#include <gtest/gtest.h>
#include <neo/protocol_settings.h>
#include <neo/hardfork.h>
#include <fstream>
#include <filesystem>

using namespace neo;

class ProtocolSettingsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test configuration file
        testConfigPath = "test_protocol_config.json";
        CreateTestConfigFile();
    }

    void TearDown() override
    {
        // Clean up test files
        if (std::filesystem::exists(testConfigPath))
        {
            std::filesystem::remove(testConfigPath);
        }
    }

    void CreateTestConfigFile()
    {
        std::string testConfig = R"({
            "ProtocolConfiguration": {
                "Network": 860833102,
                "AddressVersion": 53,
                "MillisecondsPerBlock": 15000,
                "MaxTransactionsPerBlock": 512,
                "MemoryPoolMaxTransactions": 50000,
                "MaxTraceableBlocks": 2102400,
                "MaxValidUntilBlockIncrement": 5760,
                "InitialGasDistribution": 5200000000000000,
                "ValidatorsCount": 7,
                "StandbyCommittee": [
                    "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
                    "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
                    "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a",
                    "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
                    "024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d",
                    "02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e",
                    "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70"
                ],
                "SeedList": [
                    "seed1.neo.org:10333",
                    "seed2.neo.org:10333",
                    "seed3.neo.org:10333",
                    "seed4.neo.org:10333",
                    "seed5.neo.org:10333"
                ],
                "Hardforks": {
                    "HF_Aspidochelone": 0,
                    "HF_Basilisk": 4120000
                }
            }
        })";

        std::ofstream file(testConfigPath);
        file << testConfig;
        file.close();
    }

    std::string testConfigPath;
};

// Test default constructor
TEST_F(ProtocolSettingsTest, DefaultConstructor)
{
    ProtocolSettings settings;
    
    // Test default values match C# implementation
    EXPECT_EQ(0x334F454E, settings.GetNetwork()); // MainNet magic
    EXPECT_EQ(0x35, settings.GetAddressVersion());
    EXPECT_EQ(2102400u, settings.GetMaxTraceableBlocks());
    EXPECT_EQ(15000u, settings.GetMillisecondsPerBlock());
    EXPECT_EQ(512u, settings.GetMaxTransactionsPerBlock());
    EXPECT_EQ(50000, settings.GetMemoryPoolMaxTransactions());
    EXPECT_EQ(5200000000000000ull, settings.GetInitialGasDistribution());
}

// Test copy constructor and assignment
TEST_F(ProtocolSettingsTest, CopyConstructorAndAssignment)
{
    ProtocolSettings original;
    original.SetNetwork(12345);
    original.SetAddressVersion(42);
    
    // Test copy constructor
    ProtocolSettings copied(original);
    EXPECT_EQ(original.GetNetwork(), copied.GetNetwork());
    EXPECT_EQ(original.GetAddressVersion(), copied.GetAddressVersion());
    
    // Test assignment operator
    ProtocolSettings assigned;
    assigned = original;
    EXPECT_EQ(original.GetNetwork(), assigned.GetNetwork());
    EXPECT_EQ(original.GetAddressVersion(), assigned.GetAddressVersion());
}

// Test network settings
TEST_F(ProtocolSettingsTest, NetworkSettings)
{
    ProtocolSettings settings;
    
    // Test network magic number
    uint32_t testNetwork = 0x12345678;
    settings.SetNetwork(testNetwork);
    EXPECT_EQ(testNetwork, settings.GetNetwork());
    
    // Test address version
    uint8_t testAddressVersion = 0x42;
    settings.SetAddressVersion(testAddressVersion);
    EXPECT_EQ(testAddressVersion, settings.GetAddressVersion());
}

// Test block and transaction settings
TEST_F(ProtocolSettingsTest, BlockAndTransactionSettings)
{
    ProtocolSettings settings;
    
    // Test milliseconds per block
    uint32_t testMilliseconds = 10000;
    settings.SetMillisecondsPerBlock(testMilliseconds);
    EXPECT_EQ(testMilliseconds, settings.GetMillisecondsPerBlock());
    
    // Test max transactions per block
    uint32_t testMaxTx = 1000;
    settings.SetMaxTransactionsPerBlock(testMaxTx);
    EXPECT_EQ(testMaxTx, settings.GetMaxTransactionsPerBlock());
    
    // Test max valid until block increment
    uint32_t testMaxIncrement = 86400;
    settings.SetMaxValidUntilBlockIncrement(testMaxIncrement);
    EXPECT_EQ(testMaxIncrement, settings.GetMaxValidUntilBlockIncrement());
    
    // Test memory pool max transactions
    int testMemPoolMax = 100000;
    settings.SetMemoryPoolMaxTransactions(testMemPoolMax);
    EXPECT_EQ(testMemPoolMax, settings.GetMemoryPoolMaxTransactions());
    
    // Test max traceable blocks
    uint32_t testMaxTraceable = 5000000;
    settings.SetMaxTraceableBlocks(testMaxTraceable);
    EXPECT_EQ(testMaxTraceable, settings.GetMaxTraceableBlocks());
}

// Test gas distribution
TEST_F(ProtocolSettingsTest, GasDistribution)
{
    ProtocolSettings settings;
    
    uint64_t testGasDistribution = 1000000000000000ull;
    settings.SetInitialGasDistribution(testGasDistribution);
    EXPECT_EQ(testGasDistribution, settings.GetInitialGasDistribution());
}

// Test validators and committee
TEST_F(ProtocolSettingsTest, ValidatorsAndCommittee)
{
    ProtocolSettings settings;
    
    // Test validators count
    int testValidatorsCount = 21;
    settings.SetValidatorsCount(testValidatorsCount);
    EXPECT_EQ(testValidatorsCount, settings.GetValidatorsCount());
    
    // Test committee members count
    std::vector<neo::cryptography::ECPoint> testCommittee;
    // Add test ECPoints here when ECPoint class is available
    settings.SetStandbyCommittee(testCommittee);
    EXPECT_EQ(testCommittee.size(), settings.GetCommitteeMembersCount());
}

// Test seed list
TEST_F(ProtocolSettingsTest, SeedList)
{
    ProtocolSettings settings;
    
    std::vector<std::string> testSeeds = {
        "seed1.example.com:10333",
        "seed2.example.com:10333",
        "seed3.example.com:10333"
    };
    
    settings.SetSeedList(testSeeds);
    const auto& retrievedSeeds = settings.GetSeedList();
    
    EXPECT_EQ(testSeeds.size(), retrievedSeeds.size());
    for (size_t i = 0; i < testSeeds.size(); ++i)
    {
        EXPECT_EQ(testSeeds[i], retrievedSeeds[i]);
    }
}

// Test hardfork functionality
TEST_F(ProtocolSettingsTest, HardforkConfiguration)
{
    ProtocolSettings settings;
    
    std::unordered_map<Hardfork, uint32_t> testHardforks = {
        {Hardfork::HF_Aspidochelone, 0},
        {Hardfork::HF_Basilisk, 4120000},
        {Hardfork::HF_Cockatrice, 5000000}
    };
    
    settings.SetHardforks(testHardforks);
    const auto& retrievedHardforks = settings.GetHardforks();
    
    EXPECT_EQ(testHardforks.size(), retrievedHardforks.size());
    for (const auto& [hardfork, height] : testHardforks)
    {
        auto it = retrievedHardforks.find(hardfork);
        ASSERT_NE(it, retrievedHardforks.end());
        EXPECT_EQ(height, it->second);
    }
}

// Test hardfork enabled logic
TEST_F(ProtocolSettingsTest, HardforkEnabledLogic)
{
    ProtocolSettings settings;
    
    std::unordered_map<Hardfork, uint32_t> hardforks = {
        {Hardfork::HF_Aspidochelone, 0},
        {Hardfork::HF_Basilisk, 4120000}
    };
    settings.SetHardforks(hardforks);
    
    // Test Aspidochelone (enabled from block 0)
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 1000000));
    
    // Test Basilisk (enabled from block 4120000)
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 4119999));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 4120000));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 5000000));
    
    // Test undefined hardfork
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Cockatrice, 0));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Cockatrice, 10000000));
}

// Test JSON configuration loading
TEST_F(ProtocolSettingsTest, JSONConfigurationLoading)
{
    auto settings = ProtocolSettings::Load(testConfigPath);
    ASSERT_NE(nullptr, settings);
    
    // Verify loaded values match test configuration
    EXPECT_EQ(860833102u, settings->GetNetwork());
    EXPECT_EQ(53, settings->GetAddressVersion());
    EXPECT_EQ(15000u, settings->GetMillisecondsPerBlock());
    EXPECT_EQ(512u, settings->GetMaxTransactionsPerBlock());
    EXPECT_EQ(50000, settings->GetMemoryPoolMaxTransactions());
    EXPECT_EQ(2102400u, settings->GetMaxTraceableBlocks());
    EXPECT_EQ(5760u, settings->GetMaxValidUntilBlockIncrement());
    EXPECT_EQ(5200000000000000ull, settings->GetInitialGasDistribution());
    EXPECT_EQ(7, settings->GetValidatorsCount());
    
    // Test seed list
    const auto& seedList = settings->GetSeedList();
    EXPECT_EQ(5u, seedList.size());
    EXPECT_EQ("seed1.neo.org:10333", seedList[0]);
    
    // Test hardforks
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 4120000));
    EXPECT_FALSE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 4119999));
}

// Test hardfork validation
TEST_F(ProtocolSettingsTest, HardforkValidation)
{
    ProtocolSettings settings;
    
    // Test valid hardfork configuration
    std::unordered_map<Hardfork, uint32_t> validHardforks = {
        {Hardfork::HF_Aspidochelone, 0},
        {Hardfork::HF_Basilisk, 1000000}
    };
    settings.SetHardforks(validHardforks);
    EXPECT_NO_THROW(settings.ValidateHardforkConfiguration());
    
    // Test invalid hardfork configuration (decreasing heights)
    std::unordered_map<Hardfork, uint32_t> invalidHardforks = {
        {Hardfork::HF_Aspidochelone, 1000000},
        {Hardfork::HF_Basilisk, 500000}  // Earlier than previous
    };
    settings.SetHardforks(invalidHardforks);
    EXPECT_THROW(settings.ValidateHardforkConfiguration(), std::invalid_argument);
}

// Test default settings
TEST_F(ProtocolSettingsTest, DefaultSettings)
{
    const auto& defaultSettings = ProtocolSettings::GetDefault();
    
    // Verify default settings match C# implementation
    EXPECT_EQ(0u, defaultSettings.GetNetwork()); // Default network
    EXPECT_EQ(0x35, defaultSettings.GetAddressVersion());
    EXPECT_TRUE(defaultSettings.GetStandbyCommittee().empty()); // Default empty
    EXPECT_EQ(0, defaultSettings.GetValidatorsCount());
    EXPECT_TRUE(defaultSettings.GetSeedList().empty()); // Default empty
    EXPECT_EQ(15000u, defaultSettings.GetMillisecondsPerBlock());
    EXPECT_EQ(512u, defaultSettings.GetMaxTransactionsPerBlock());
    EXPECT_EQ(5760u, defaultSettings.GetMaxValidUntilBlockIncrement());
    EXPECT_EQ(50000, defaultSettings.GetMemoryPoolMaxTransactions());
    EXPECT_EQ(2102400u, defaultSettings.GetMaxTraceableBlocks());
    EXPECT_EQ(5200000000000000ull, defaultSettings.GetInitialGasDistribution());
}

// Test address format validation (requires address generation)
TEST_F(ProtocolSettingsTest, AddressFormatValidation)
{
    const auto& settings = ProtocolSettings::GetDefault();
    
    // Test that addresses generated with this version start with 'N'
    // This test would require UInt160::ToAddress implementation
    // EXPECT_EQ('N', someUInt160.ToAddress(settings.GetAddressVersion())[0]);
}

// Performance test for hardfork checking
TEST_F(ProtocolSettingsTest, HardforkPerformance)
{
    ProtocolSettings settings;
    std::unordered_map<Hardfork, uint32_t> hardforks = {
        {Hardfork::HF_Aspidochelone, 0},
        {Hardfork::HF_Basilisk, 4120000},
        {Hardfork::HF_Cockatrice, 5000000},
        {Hardfork::HF_Domovoi, 6000000},
        {Hardfork::HF_Echidna, 7000000}
    };
    settings.SetHardforks(hardforks);
    
    // Performance test - should be very fast
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i)
    {
        settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 5000000);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 100); // Should complete in less than 100ms
} 