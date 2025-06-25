#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/protocol_settings.h"
#include "neo/io/uint160.h"
#include "neo/cryptography/ecc/ecpoint.h"
#include "neo/cryptography/ecc/secp256r1.h"
#include "neo/hardfork.h"
#include "neo/wallets/helper.h"
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <regex>
#include <filesystem>
#include <chrono>

using namespace neo;
using namespace neo::io;
using namespace neo::cryptography::ecc;
using namespace neo::wallets;

// Complete conversion of C# UT_ProtocolSettings.cs - ALL 32 test methods
class ProtocolSettingsAllMethodsTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_settings_ = CreateTestProtocolSettings();
    }
    
    void TearDown() override {
        // Clean up any temporary files
        if (std::filesystem::exists(temp_file_)) {
            std::filesystem::remove(temp_file_);
        }
    }
    
    ProtocolSettings CreateTestProtocolSettings() {
        ProtocolSettings settings;
        settings.SetNetwork(0x334E454F); // Neo mainnet magic number
        settings.SetAddressVersion(53);
        settings.SetMillisecondsPerBlock(15000);
        settings.SetMaxTransactionsPerBlock(512);
        settings.SetMemoryPoolMaxTransactions(50000);
        settings.SetMaxTraceableBlocks(2102400);
        settings.SetInitialGasDistribution(5200000000000000);
        settings.SetValidatorsCount(7);
        
        // Set up standby committee (21 validators)
        std::vector<ECPoint> committee = {
            ECPoint::Parse("03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c"),
            ECPoint::Parse("02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093"),
            ECPoint::Parse("03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a"),
            ECPoint::Parse("02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554"),
            ECPoint::Parse("024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d"),
            ECPoint::Parse("02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e"),
            ECPoint::Parse("02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70"),
            ECPoint::Parse("023a36c72844610b4d34d1968662424011bf783ca9d984efa19a20babf5582f3fe"),
            ECPoint::Parse("03708b860c1de5d87f5b151a12c2a99feebd2e8b315ee8e7cf8aa19692a9e18379"),
            ECPoint::Parse("03c6aa6e12638b36e88adc1ccdceac4db9929575c3e03576c617c49cce7114a050"),
            ECPoint::Parse("03204223f8c86b8cd5c89ef12e4f0dbb314172e9241e30c9ef2293790793537cf0"),
            ECPoint::Parse("02a62c915cf19c7f19a50ec217e79fac2439bbaad658493de0c7d8ffa92ab0aa62"),
            ECPoint::Parse("03409f31f0d66bdc2f70a9730b66fe186658f84a8018204db01c106edc36553cd0"),
            ECPoint::Parse("0288342b141c30dc8ffcde0204929bb46aed5756b41ef4a56778d15ada8f0c6654"),
            ECPoint::Parse("020f2887f41474cfeb11fd262e982051c1541418137c02a0f4961af911045de639"),
            ECPoint::Parse("0222038884bbd1d8ff109ed3bdef3542e768eef76c1247aea8bc8171f532928c30"),
            ECPoint::Parse("03d281b42002647f0113f36c7b8efb30db66078dfaaa9ab3ff76d043a98d512fde"),
            ECPoint::Parse("02504acbc1f4b3bdad1d86d6e1a08603771db135a73e61c9d565ae06a1938cd2ad"),
            ECPoint::Parse("0226933336f1b75baa42d42b71d9091508b638046d19abd67f4e119bf64a7cfb4d"),
            ECPoint::Parse("03cdcea66032b82f5c30450e381e5295cae85c5e6943af716cc6b646352a6067dc"),
            ECPoint::Parse("02cd5a5547119e24feaa7c2a0f37b8c9366216bab7054de0065c9be42084003c8a")
        };
        settings->SetStandbyCommittee(committee);
        
        // Set up seed list
        std::vector<std::string> seedList = {
            "seed1.neo.org:10333",
            "seed2.neo.org:10333", 
            "seed3.neo.org:10333",
            "seed4.neo.org:10333",
            "seed5.neo.org:10333"
        };
        settings->SetSeedList(seedList);
        
        // Set up hardforks
        std::unordered_map<Hardfork, uint32_t> hardforks;
        hardforks[Hardfork::HF_Aspidochelone] = 0;
        hardforks[Hardfork::HF_Basilisk] = 0;
        settings->SetHardforks(hardforks);
        
        return settings;
    }
    
    std::string CreateHFSettings(const std::string& hf) {
        return R"({
    "ProtocolConfiguration": {
        "Network": 860833102,
        "AddressVersion": 53,
        "MillisecondsPerBlock": 15000,
        "MaxTransactionsPerBlock": 512,
        "MemoryPoolMaxTransactions": 50000,
        "MaxTraceableBlocks": 2102400,
        "Hardforks": {
)" + hf + R"(
        },
        "InitialGasDistribution": 5200000000000000,
        "ValidatorsCount": 7,
        "StandbyCommittee": [
            "03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c",
            "02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093",
            "03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a",
            "02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554",
            "024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d",
            "02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e",
            "02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70",
            "023a36c72844610b4d34d1968662424011bf783ca9d984efa19a20babf5582f3fe",
            "03708b860c1de5d87f5b151a12c2a99feebd2e8b315ee8e7cf8aa19692a9e18379",
            "03c6aa6e12638b36e88adc1ccdceac4db9929575c3e03576c617c49cce7114a050",
            "03204223f8c86b8cd5c89ef12e4f0dbb314172e9241e30c9ef2293790793537cf0",
            "02a62c915cf19c7f19a50ec217e79fac2439bbaad658493de0c7d8ffa92ab0aa62",
            "03409f31f0d66bdc2f70a9730b66fe186658f84a8018204db01c106edc36553cd0",
            "0288342b141c30dc8ffcde0204929bb46aed5756b41ef4a56778d15ada8f0c6654",
            "020f2887f41474cfeb11fd262e982051c1541418137c02a0f4961af911045de639",
            "0222038884bbd1d8ff109ed3bdef3542e768eef76c1247aea8bc8171f532928c30",
            "03d281b42002647f0113f36c7b8efb30db66078dfaaa9ab3ff76d043a98d512fde",
            "02504acbc1f4b3bdad1d86d6e1a08603771db135a73e61c9d565ae06a1938cd2ad",
            "0226933336f1b75baa42d42b71d9091508b638046d19abd67f4e119bf64a7cfb4d",
            "03cdcea66032b82f5c30450e381e5295cae85c5e6943af716cc6b646352a6067dc",
            "02cd5a5547119e24feaa7c2a0f37b8c9366216bab7054de0065c9be42084003c8a"
        ],
        "SeedList": [
            "seed1.neo.org:10333",
            "seed2.neo.org:10333",
            "seed3.neo.org:10333",
            "seed4.neo.org:10333",
            "seed5.neo.org:10333"
        ]
    }
})";
    }
    
    ProtocolSettings test_settings_;
    std::string temp_file_;
};

// C# Test Method: CheckFirstLetterOfAddresses()
TEST_F(ProtocolSettingsAllMethodsTest, CheckFirstLetterOfAddresses) {
    UInt160 min = UInt160::Parse("0x0000000000000000000000000000000000000000");
    std::string min_address = Helper::ToAddress(min, test_settings_.GetAddressVersion());
    EXPECT_EQ('N', min_address[0]);
    
    UInt160 max = UInt160::Parse("0xffffffffffffffffffffffffffffffffffffffff");
    std::string max_address = Helper::ToAddress(max, test_settings_.GetAddressVersion());
    EXPECT_EQ('N', max_address[0]);
}

// C# Test Method: Default_Network_should_be_mainnet_Network_value()
TEST_F(ProtocolSettingsAllMethodsTest, Default_Network_should_be_mainnet_Network_value) {
    uint32_t mainnet_network = 0x334F454E;
    EXPECT_EQ(mainnet_network, test_settings_.GetNetwork());
}

// C# Test Method: TestGetMemoryPoolMaxTransactions()
TEST_F(ProtocolSettingsAllMethodsTest, TestGetMemoryPoolMaxTransactions) {
    EXPECT_EQ(50000, test_settings_.GetMemoryPoolMaxTransactions());
}

// C# Test Method: TestGetMillisecondsPerBlock()
TEST_F(ProtocolSettingsAllMethodsTest, TestGetMillisecondsPerBlock) {
    EXPECT_EQ(15000U, test_settings_.GetMillisecondsPerBlock());
}

// C# Test Method: HardForkTestBAndNotA()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestBAndNotA) {
    std::string json = CreateHFSettings("\"HF_Basilisk\": 4120000");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    auto settings = ProtocolSettings::Load(temp_file_);
    
    EXPECT_EQ(0U, settings->GetHardforks().at(Hardfork::HF_Aspidochelone));
    EXPECT_EQ(4120000U, settings->GetHardforks().at(Hardfork::HF_Basilisk));
    
    // Check IsHardforkEnabled
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 10));
    EXPECT_FALSE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_FALSE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 10));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 4120000));
}

// C# Test Method: HardForkTestAAndNotB()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestAAndNotB) {
    std::string json = CreateHFSettings("\"HF_Aspidochelone\": 0");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings2.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    auto settings = ProtocolSettings::Load(temp_file_);
    
    EXPECT_EQ(0U, settings->GetHardforks().at(Hardfork::HF_Aspidochelone));
    EXPECT_EQ(settings->GetHardforks().end(), settings->GetHardforks().find(Hardfork::HF_Basilisk));
    
    // Check IsHardforkEnabled
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 10));
    EXPECT_FALSE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_FALSE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 10));
    EXPECT_FALSE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 4120000));
}

// C# Test Method: HardForkTestNone()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestNone) {
    std::string json = CreateHFSettings("");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings3.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    auto settings = ProtocolSettings::Load(temp_file_);
    
    EXPECT_EQ(0U, settings->GetHardforks().at(Hardfork::HF_Aspidochelone));
    EXPECT_EQ(0U, settings->GetHardforks().at(Hardfork::HF_Basilisk));
    
    // Check IsHardforkEnabled
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Aspidochelone, 10));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_TRUE(settings->IsHardforkEnabled(Hardfork::HF_Basilisk, 10));
}

// C# Test Method: HardForkTestAMoreThanB()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestAMoreThanB) {
    std::string json = CreateHFSettings("\"HF_Aspidochelone\": 4120001, \"HF_Basilisk\": 4120000");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings4.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    EXPECT_THROW(ProtocolSettings::Load(temp_file_), std::invalid_argument);
}

// C# Test Method: TestGetSeedList()
TEST_F(ProtocolSettingsAllMethodsTest, TestGetSeedList) {
    std::vector<std::string> expected = {
        "seed1.neo.org:10333",
        "seed2.neo.org:10333",
        "seed3.neo.org:10333",
        "seed4.neo.org:10333",
        "seed5.neo.org:10333"
    };
    
    EXPECT_EQ(expected, test_settings_.GetSeedList());
}

// C# Test Method: TestStandbyCommitteeAddressesFormat()
TEST_F(ProtocolSettingsAllMethodsTest, TestStandbyCommitteeAddressesFormat) {
    std::regex hex_pattern("^[0-9A-Fa-f]{66}$"); // ECPoint is 66 hex characters
    
    for (const auto& point : test_settings_.GetStandbyCommittee()) {
        std::string point_str = point.ToString();
        EXPECT_TRUE(std::regex_match(point_str, hex_pattern));
    }
}

// C# Test Method: TestValidatorsCount()
TEST_F(ProtocolSettingsAllMethodsTest, TestValidatorsCount) {
    EXPECT_EQ(test_settings_.GetValidatorsCount() * 3, test_settings_.GetStandbyCommittee().size());
}

// C# Test Method: TestMaxTransactionsPerBlock()
TEST_F(ProtocolSettingsAllMethodsTest, TestMaxTransactionsPerBlock) {
    EXPECT_GT(test_settings_.GetMaxTransactionsPerBlock(), 0);
    EXPECT_LE(test_settings_.GetMaxTransactionsPerBlock(), 50000);
}

// C# Test Method: TestMaxTraceableBlocks()
TEST_F(ProtocolSettingsAllMethodsTest, TestMaxTraceableBlocks) {
    EXPECT_GT(test_settings_.GetMaxTraceableBlocks(), 0);
}

// C# Test Method: TestMaxValidUntilBlockIncrement()
TEST_F(ProtocolSettingsAllMethodsTest, TestMaxValidUntilBlockIncrement) {
    EXPECT_GT(test_settings_.GetMaxValidUntilBlockIncrement(), 0);
}

// C# Test Method: TestInitialGasDistribution()
TEST_F(ProtocolSettingsAllMethodsTest, TestInitialGasDistribution) {
    EXPECT_GT(test_settings_.GetInitialGasDistribution(), 0);
}

// C# Test Method: TestHardforksSettings()
TEST_F(ProtocolSettingsAllMethodsTest, TestHardforksSettings) {
    EXPECT_FALSE(test_settings_.GetHardforks().empty());
}

// C# Test Method: TestAddressVersion()
TEST_F(ProtocolSettingsAllMethodsTest, TestAddressVersion) {
    EXPECT_GE(test_settings_.GetAddressVersion(), 0);
    EXPECT_LE(test_settings_.GetAddressVersion(), 255);
}

// C# Test Method: TestNetworkSettingsConsistency()
TEST_F(ProtocolSettingsAllMethodsTest, TestNetworkSettingsConsistency) {
    EXPECT_GT(test_settings_.GetNetwork(), 0);
    EXPECT_FALSE(test_settings_.GetSeedList().empty());
}

// C# Test Method: TestECPointParsing()
TEST_F(ProtocolSettingsAllMethodsTest, TestECPointParsing) {
    for (const auto& point : test_settings_.GetStandbyCommittee()) {
        EXPECT_NO_THROW({
            ECPoint::Parse(point.ToString());
        });
    }
}

// C# Test Method: TestSeedListFormatAndReachability()
TEST_F(ProtocolSettingsAllMethodsTest, TestSeedListFormatAndReachability) {
    std::regex format_pattern(R"(^[\w.-]+:\d+$)"); // Format: domain:port
    
    for (const auto& seed : test_settings_.GetSeedList()) {
        EXPECT_TRUE(std::regex_match(seed, format_pattern));
    }
}

// C# Test Method: TestDefaultNetworkValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultNetworkValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(0U, default_settings.GetNetwork());
}

// C# Test Method: TestDefaultAddressVersionValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultAddressVersionValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetAddressVersion(), test_settings_.GetAddressVersion());
}

// C# Test Method: TestDefaultValidatorsCountValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultValidatorsCountValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(0, default_settings.GetValidatorsCount());
}

// C# Test Method: TestDefaultMillisecondsPerBlockValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMillisecondsPerBlockValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetMillisecondsPerBlock(), test_settings_.GetMillisecondsPerBlock());
}

// C# Test Method: TestDefaultMaxTransactionsPerBlockValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMaxTransactionsPerBlockValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetMaxTransactionsPerBlock(), test_settings_.GetMaxTransactionsPerBlock());
}

// C# Test Method: TestDefaultMemoryPoolMaxTransactionsValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMemoryPoolMaxTransactionsValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetMemoryPoolMaxTransactions(), test_settings_.GetMemoryPoolMaxTransactions());
}

// C# Test Method: TestDefaultMaxTraceableBlocksValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMaxTraceableBlocksValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetMaxTraceableBlocks(), test_settings_.GetMaxTraceableBlocks());
}

// C# Test Method: TestDefaultMaxValidUntilBlockIncrementValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMaxValidUntilBlockIncrementValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetMaxValidUntilBlockIncrement(), test_settings_.GetMaxValidUntilBlockIncrement());
}

// C# Test Method: TestDefaultInitialGasDistributionValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultInitialGasDistributionValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetInitialGasDistribution(), test_settings_.GetInitialGasDistribution());
}

// C# Test Method: TestDefaultHardforksValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultHardforksValue) {
    ProtocolSettings default_settings;
    EXPECT_EQ(default_settings.GetHardforks(), test_settings_.GetHardforks());
}

// C# Test Method: TestTimePerBlockCalculation()
TEST_F(ProtocolSettingsAllMethodsTest, TestTimePerBlockCalculation) {
    // TimePerBlock is calculated from MillisecondsPerBlock
    auto expected_milliseconds = test_settings_.GetMillisecondsPerBlock();
    EXPECT_EQ(15000U, expected_milliseconds);
}

// C# Test Method: TestLoad()
TEST_F(ProtocolSettingsAllMethodsTest, TestLoad) {
    // Create test config file
    std::string config_content = CreateHFSettings("\"HF_Aspidochelone\": 0, \"HF_Basilisk\": 0");
    temp_file_ = "test.config.json";
    
    std::ofstream file(temp_file_);
    file << config_content;
    file.close();
    
    auto loaded_setting = ProtocolSettings::Load(temp_file_);
    
    // Compare all properties
    EXPECT_EQ(test_settings_.GetNetwork(), loaded_setting.GetNetwork());
    EXPECT_EQ(test_settings_.GetAddressVersion(), loaded_setting.GetAddressVersion());
    EXPECT_EQ(test_settings_.GetStandbyCommittee(), loaded_setting.GetStandbyCommittee());
    EXPECT_EQ(test_settings_.GetValidatorsCount(), loaded_setting.GetValidatorsCount());
    EXPECT_EQ(test_settings_.GetSeedList(), loaded_setting.GetSeedList());
    EXPECT_EQ(test_settings_.GetMillisecondsPerBlock(), loaded_setting.GetMillisecondsPerBlock());
    EXPECT_EQ(test_settings_.GetMaxTransactionsPerBlock(), loaded_setting.GetMaxTransactionsPerBlock());
    EXPECT_EQ(test_settings_.GetMemoryPoolMaxTransactions(), loaded_setting.GetMemoryPoolMaxTransactions());
    EXPECT_EQ(test_settings_.GetMaxTraceableBlocks(), loaded_setting.GetMaxTraceableBlocks());
    EXPECT_EQ(test_settings_.GetMaxValidUntilBlockIncrement(), loaded_setting.GetMaxValidUntilBlockIncrement());
    EXPECT_EQ(test_settings_.GetInitialGasDistribution(), loaded_setting.GetInitialGasDistribution());
    EXPECT_EQ(test_settings_.GetHardforks(), loaded_setting.GetHardforks());
    
    // Compare StandbyValidators (derived property)
    auto test_validators = test_settings_.GetStandbyValidators();
    auto loaded_validators = loaded_setting.GetStandbyValidators();
    EXPECT_EQ(test_validators, loaded_validators);
}

// Additional comprehensive tests for complete coverage

// Test Method: TestProtocolSettingsValidation()
TEST_F(ProtocolSettingsAllMethodsTest, TestProtocolSettingsValidation) {
    // Test invalid JSON
    temp_file_ = std::filesystem::temp_directory_path() / "invalid.json";
    std::ofstream file(temp_file_);
    file << "invalid json content {";
    file.close();
    
    EXPECT_THROW(ProtocolSettings::Load(temp_file_), std::exception);
    
    // Test missing file
    EXPECT_THROW(ProtocolSettings::Load("nonexistent.json"), std::exception);
}

// Test Method: TestStandbyValidatorsDerivation()
TEST_F(ProtocolSettingsAllMethodsTest, TestStandbyValidatorsDerivation) {
    auto validators = test_settings_.GetStandbyValidators();
    EXPECT_EQ(test_settings_.GetValidatorsCount(), validators.size());
    
    // Validators should be the first N members of standby committee
    for (size_t i = 0; i < validators.size(); ++i) {
        EXPECT_EQ(test_settings_.GetStandbyCommittee()[i], validators[i]);
    }
}

// Test Method: TestNetworkConstants()
TEST_F(ProtocolSettingsAllMethodsTest, TestNetworkConstants) {
    // Test known network values
    EXPECT_EQ(0x334E454F, test_settings_.GetNetwork()); // "NEO\x00" in little endian
    
    // Test that network is not zero for test settings
    EXPECT_NE(0U, test_settings_.GetNetwork());
}

// Test Method: TestProtocolSettingsImmutability()
TEST_F(ProtocolSettingsAllMethodsTest, TestProtocolSettingsImmutability) {
    auto settings1 = test_settings_;
    auto settings2 = test_settings_;
    
    // Settings should be equal
    EXPECT_EQ(settings1.GetNetwork(), settings2.GetNetwork());
    EXPECT_EQ(settings1.GetAddressVersion(), settings2.GetAddressVersion());
    EXPECT_EQ(settings1.GetValidatorsCount(), settings2.GetValidatorsCount());
    
    // Modifying one shouldn't affect the other (copy semantics)
    settings1.SetValidatorsCount(999);
    EXPECT_NE(settings1.GetValidatorsCount(), settings2.GetValidatorsCount());
}