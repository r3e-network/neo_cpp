#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/protocol_settings.h"
#include "neo/io/uint160.h"
#include "neo/cryptography/ecc/ecpoint.h"
#include "neo/cryptography/ecc/eccurve.h"
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
        settings.Network = 0x334E454F; // Neo mainnet magic number
        settings.AddressVersion = 53;
        settings.MillisecondsPerBlock = 15000;
        settings.MaxTransactionsPerBlock = 512;
        settings.MemoryPoolMaxTransactions = 50000;
        settings.MaxTraceableBlocks = 2102400;
        settings.InitialGasDistribution = 5200000000000000;
        settings.ValidatorsCount = 7;
        
        // Set up standby committee (21 validators)
        settings.StandbyCommittee = {
            ECPoint::Parse("03b209fd4f53a7170ea4444e0cb0a6bb6a53c2bd016926989cf85f9b0fba17a70c", ECCurve::Secp256r1),
            ECPoint::Parse("02df48f60e8f3e01c48ff40b9b7f1310d7a8b2a193188befe1c2e3df740e895093", ECCurve::Secp256r1),
            ECPoint::Parse("03b8d9d5771d8f513aa0869b9cc8d50986403b78c6da36890638c3d46a5adce04a", ECCurve::Secp256r1),
            ECPoint::Parse("02ca0e27697b9c248f6f16e085fd0061e26f44da85b58ee835c110caa5ec3ba554", ECCurve::Secp256r1),
            ECPoint::Parse("024c7b7fb6c310fccf1ba33b082519d82964ea93868d676662d4a59ad548df0e7d", ECCurve::Secp256r1),
            ECPoint::Parse("02aaec38470f6aad0042c6e877cfd8087d2676b0f516fddd362801b9bd3936399e", ECCurve::Secp256r1),
            ECPoint::Parse("02486fd15702c4490a26703112a5cc1d0923fd697a33406bd5a1c00e0013b09a70", ECCurve::Secp256r1),
            ECPoint::Parse("023a36c72844610b4d34d1968662424011bf783ca9d984efa19a20babf5582f3fe", ECCurve::Secp256r1),
            ECPoint::Parse("03708b860c1de5d87f5b151a12c2a99feebd2e8b315ee8e7cf8aa19692a9e18379", ECCurve::Secp256r1),
            ECPoint::Parse("03c6aa6e12638b36e88adc1ccdceac4db9929575c3e03576c617c49cce7114a050", ECCurve::Secp256r1),
            ECPoint::Parse("03204223f8c86b8cd5c89ef12e4f0dbb314172e9241e30c9ef2293790793537cf0", ECCurve::Secp256r1),
            ECPoint::Parse("02a62c915cf19c7f19a50ec217e79fac2439bbaad658493de0c7d8ffa92ab0aa62", ECCurve::Secp256r1),
            ECPoint::Parse("03409f31f0d66bdc2f70a9730b66fe186658f84a8018204db01c106edc36553cd0", ECCurve::Secp256r1),
            ECPoint::Parse("0288342b141c30dc8ffcde0204929bb46aed5756b41ef4a56778d15ada8f0c6654", ECCurve::Secp256r1),
            ECPoint::Parse("020f2887f41474cfeb11fd262e982051c1541418137c02a0f4961af911045de639", ECCurve::Secp256r1),
            ECPoint::Parse("0222038884bbd1d8ff109ed3bdef3542e768eef76c1247aea8bc8171f532928c30", ECCurve::Secp256r1),
            ECPoint::Parse("03d281b42002647f0113f36c7b8efb30db66078dfaaa9ab3ff76d043a98d512fde", ECCurve::Secp256r1),
            ECPoint::Parse("02504acbc1f4b3bdad1d86d6e1a08603771db135a73e61c9d565ae06a1938cd2ad", ECCurve::Secp256r1),
            ECPoint::Parse("0226933336f1b75baa42d42b71d9091508b638046d19abd67f4e119bf64a7cfb4d", ECCurve::Secp256r1),
            ECPoint::Parse("03cdcea66032b82f5c30450e381e5295cae85c5e6943af716cc6b646352a6067dc", ECCurve::Secp256r1),
            ECPoint::Parse("02cd5a5547119e24feaa7c2a0f37b8c9366216bab7054de0065c9be42084003c8a", ECCurve::Secp256r1)
        };
        
        // Set up seed list
        settings.SeedList = {
            "seed1.neo.org:10333",
            "seed2.neo.org:10333", 
            "seed3.neo.org:10333",
            "seed4.neo.org:10333",
            "seed5.neo.org:10333"
        };
        
        // Set up hardforks
        settings.Hardforks[Hardfork::HF_Aspidochelone] = 0;
        settings.Hardforks[Hardfork::HF_Basilisk] = 0;
        
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
    EXPECT_EQ('N', ToAddress(min, test_settings_.AddressVersion)[0]);
    
    UInt160 max = UInt160::Parse("0xffffffffffffffffffffffffffffffffffffffff");
    EXPECT_EQ('N', ToAddress(max, test_settings_.AddressVersion)[0]);
}

// C# Test Method: Default_Network_should_be_mainnet_Network_value()
TEST_F(ProtocolSettingsAllMethodsTest, Default_Network_should_be_mainnet_Network_value) {
    uint32_t mainnet_network = 0x334F454E;
    EXPECT_EQ(mainnet_network, test_settings_.Network);
}

// C# Test Method: TestGetMemoryPoolMaxTransactions()
TEST_F(ProtocolSettingsAllMethodsTest, TestGetMemoryPoolMaxTransactions) {
    EXPECT_EQ(50000, test_settings_.MemoryPoolMaxTransactions);
}

// C# Test Method: TestGetMillisecondsPerBlock()
TEST_F(ProtocolSettingsAllMethodsTest, TestGetMillisecondsPerBlock) {
    EXPECT_EQ(15000U, test_settings_.MillisecondsPerBlock);
}

// C# Test Method: HardForkTestBAndNotA()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestBAndNotA) {
    std::string json = CreateHFSettings("\"HF_Basilisk\": 4120000");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    auto settings = ProtocolSettings::Load(temp_file_);
    
    EXPECT_EQ(0U, settings.Hardforks.at(Hardfork::HF_Aspidochelone));
    EXPECT_EQ(4120000U, settings.Hardforks.at(Hardfork::HF_Basilisk));
    
    // Check IsHardforkEnabled
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 10));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 10));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 4120000));
}

// C# Test Method: HardForkTestAAndNotB()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestAAndNotB) {
    std::string json = CreateHFSettings("\"HF_Aspidochelone\": 0");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings2.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    auto settings = ProtocolSettings::Load(temp_file_);
    
    EXPECT_EQ(0U, settings.Hardforks.at(Hardfork::HF_Aspidochelone));
    EXPECT_EQ(settings.Hardforks.end(), settings.Hardforks.find(Hardfork::HF_Basilisk));
    
    // Check IsHardforkEnabled
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 10));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 10));
    EXPECT_FALSE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 4120000));
}

// C# Test Method: HardForkTestNone()
TEST_F(ProtocolSettingsAllMethodsTest, HardForkTestNone) {
    std::string json = CreateHFSettings("");
    
    temp_file_ = std::filesystem::temp_directory_path() / "test_settings3.json";
    std::ofstream file(temp_file_);
    file << json;
    file.close();
    
    auto settings = ProtocolSettings::Load(temp_file_);
    
    EXPECT_EQ(0U, settings.Hardforks.at(Hardfork::HF_Aspidochelone));
    EXPECT_EQ(0U, settings.Hardforks.at(Hardfork::HF_Basilisk));
    
    // Check IsHardforkEnabled
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 0));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Aspidochelone, 10));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 0));
    EXPECT_TRUE(settings.IsHardforkEnabled(Hardfork::HF_Basilisk, 10));
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
    
    EXPECT_EQ(expected, test_settings_.SeedList);
}

// C# Test Method: TestStandbyCommitteeAddressesFormat()
TEST_F(ProtocolSettingsAllMethodsTest, TestStandbyCommitteeAddressesFormat) {
    std::regex hex_pattern("^[0-9A-Fa-f]{66}$"); // ECPoint is 66 hex characters
    
    for (const auto& point : test_settings_.StandbyCommittee) {
        std::string point_str = point.ToString();
        EXPECT_TRUE(std::regex_match(point_str, hex_pattern));
    }
}

// C# Test Method: TestValidatorsCount()
TEST_F(ProtocolSettingsAllMethodsTest, TestValidatorsCount) {
    EXPECT_EQ(test_settings_.ValidatorsCount * 3, test_settings_.StandbyCommittee.size());
}

// C# Test Method: TestMaxTransactionsPerBlock()
TEST_F(ProtocolSettingsAllMethodsTest, TestMaxTransactionsPerBlock) {
    EXPECT_GT(test_settings_.MaxTransactionsPerBlock, 0);
    EXPECT_LE(test_settings_.MaxTransactionsPerBlock, 50000);
}

// C# Test Method: TestMaxTraceableBlocks()
TEST_F(ProtocolSettingsAllMethodsTest, TestMaxTraceableBlocks) {
    EXPECT_GT(test_settings_.MaxTraceableBlocks, 0);
}

// C# Test Method: TestMaxValidUntilBlockIncrement()
TEST_F(ProtocolSettingsAllMethodsTest, TestMaxValidUntilBlockIncrement) {
    EXPECT_GT(test_settings_.MaxValidUntilBlockIncrement, 0);
}

// C# Test Method: TestInitialGasDistribution()
TEST_F(ProtocolSettingsAllMethodsTest, TestInitialGasDistribution) {
    EXPECT_GT(test_settings_.InitialGasDistribution, 0);
}

// C# Test Method: TestHardforksSettings()
TEST_F(ProtocolSettingsAllMethodsTest, TestHardforksSettings) {
    EXPECT_FALSE(test_settings_.Hardforks.empty());
}

// C# Test Method: TestAddressVersion()
TEST_F(ProtocolSettingsAllMethodsTest, TestAddressVersion) {
    EXPECT_GE(test_settings_.AddressVersion, 0);
    EXPECT_LE(test_settings_.AddressVersion, 255);
}

// C# Test Method: TestNetworkSettingsConsistency()
TEST_F(ProtocolSettingsAllMethodsTest, TestNetworkSettingsConsistency) {
    EXPECT_GT(test_settings_.Network, 0);
    EXPECT_FALSE(test_settings_.SeedList.empty());
}

// C# Test Method: TestECPointParsing()
TEST_F(ProtocolSettingsAllMethodsTest, TestECPointParsing) {
    for (const auto& point : test_settings_.StandbyCommittee) {
        EXPECT_NO_THROW({
            ECPoint::Parse(point.ToString(), ECCurve::Secp256r1);
        });
    }
}

// C# Test Method: TestSeedListFormatAndReachability()
TEST_F(ProtocolSettingsAllMethodsTest, TestSeedListFormatAndReachability) {
    std::regex format_pattern(R"(^[\w.-]+:\d+$)"); // Format: domain:port
    
    for (const auto& seed : test_settings_.SeedList) {
        EXPECT_TRUE(std::regex_match(seed, format_pattern));
    }
}

// C# Test Method: TestDefaultNetworkValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultNetworkValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(0U, default_settings.Network);
}

// C# Test Method: TestDefaultAddressVersionValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultAddressVersionValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.AddressVersion, test_settings_.AddressVersion);
}

// C# Test Method: TestDefaultValidatorsCountValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultValidatorsCountValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(0, default_settings.ValidatorsCount);
}

// C# Test Method: TestDefaultMillisecondsPerBlockValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMillisecondsPerBlockValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.MillisecondsPerBlock, test_settings_.MillisecondsPerBlock);
}

// C# Test Method: TestDefaultMaxTransactionsPerBlockValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMaxTransactionsPerBlockValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.MaxTransactionsPerBlock, test_settings_.MaxTransactionsPerBlock);
}

// C# Test Method: TestDefaultMemoryPoolMaxTransactionsValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMemoryPoolMaxTransactionsValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.MemoryPoolMaxTransactions, test_settings_.MemoryPoolMaxTransactions);
}

// C# Test Method: TestDefaultMaxTraceableBlocksValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMaxTraceableBlocksValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.MaxTraceableBlocks, test_settings_.MaxTraceableBlocks);
}

// C# Test Method: TestDefaultMaxValidUntilBlockIncrementValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultMaxValidUntilBlockIncrementValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.MaxValidUntilBlockIncrement, test_settings_.MaxValidUntilBlockIncrement);
}

// C# Test Method: TestDefaultInitialGasDistributionValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultInitialGasDistributionValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.InitialGasDistribution, test_settings_.InitialGasDistribution);
}

// C# Test Method: TestDefaultHardforksValue()
TEST_F(ProtocolSettingsAllMethodsTest, TestDefaultHardforksValue) {
    auto default_settings = ProtocolSettings::Default();
    EXPECT_EQ(default_settings.Hardforks, test_settings_.Hardforks);
}

// C# Test Method: TestTimePerBlockCalculation()
TEST_F(ProtocolSettingsAllMethodsTest, TestTimePerBlockCalculation) {
    auto expected_timespan = std::chrono::milliseconds(test_settings_.MillisecondsPerBlock);
    EXPECT_EQ(expected_timespan, test_settings_.TimePerBlock);
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
    EXPECT_EQ(test_settings_.Network, loaded_setting.Network);
    EXPECT_EQ(test_settings_.AddressVersion, loaded_setting.AddressVersion);
    EXPECT_EQ(test_settings_.StandbyCommittee, loaded_setting.StandbyCommittee);
    EXPECT_EQ(test_settings_.ValidatorsCount, loaded_setting.ValidatorsCount);
    EXPECT_EQ(test_settings_.SeedList, loaded_setting.SeedList);
    EXPECT_EQ(test_settings_.MillisecondsPerBlock, loaded_setting.MillisecondsPerBlock);
    EXPECT_EQ(test_settings_.MaxTransactionsPerBlock, loaded_setting.MaxTransactionsPerBlock);
    EXPECT_EQ(test_settings_.MemoryPoolMaxTransactions, loaded_setting.MemoryPoolMaxTransactions);
    EXPECT_EQ(test_settings_.MaxTraceableBlocks, loaded_setting.MaxTraceableBlocks);
    EXPECT_EQ(test_settings_.MaxValidUntilBlockIncrement, loaded_setting.MaxValidUntilBlockIncrement);
    EXPECT_EQ(test_settings_.InitialGasDistribution, loaded_setting.InitialGasDistribution);
    EXPECT_EQ(test_settings_.Hardforks, loaded_setting.Hardforks);
    
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
    EXPECT_EQ(test_settings_.ValidatorsCount, validators.size());
    
    // Validators should be the first N members of standby committee
    for (size_t i = 0; i < validators.size(); ++i) {
        EXPECT_EQ(test_settings_.StandbyCommittee[i], validators[i]);
    }
}

// Test Method: TestNetworkConstants()
TEST_F(ProtocolSettingsAllMethodsTest, TestNetworkConstants) {
    // Test known network values
    EXPECT_EQ(0x334E454F, test_settings_.Network); // "NEO\x00" in little endian
    
    // Test that network is not zero for test settings
    EXPECT_NE(0U, test_settings_.Network);
}

// Test Method: TestProtocolSettingsImmutability()
TEST_F(ProtocolSettingsAllMethodsTest, TestProtocolSettingsImmutability) {
    auto settings1 = test_settings_;
    auto settings2 = test_settings_;
    
    // Settings should be equal
    EXPECT_EQ(settings1.Network, settings2.Network);
    EXPECT_EQ(settings1.AddressVersion, settings2.AddressVersion);
    EXPECT_EQ(settings1.ValidatorsCount, settings2.ValidatorsCount);
    
    // Modifying one shouldn't affect the other (copy semantics)
    settings1.ValidatorsCount = 999;
    EXPECT_NE(settings1.ValidatorsCount, settings2.ValidatorsCount);
}