#include <gtest/gtest.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/uint160.h>
#include <neo/smartcontract/contract.h>
#include <neo/smartcontract/contract_parameter_type.h>
#include <neo/wallets/wallet_account.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using namespace neo::wallets;
using namespace neo::cryptography::ecc;
using namespace neo::io;
using namespace neo::smartcontract;

/**
 * @brief Test fixture for WalletAccount
 */
class WalletAccountTest : public testing::Test
{
  protected:
    KeyPair testKeyPair;
    UInt160 testScriptHash;

    void SetUp() override
    {
        // Create a test key pair
        std::vector<uint8_t> privateKey(32, 0x01);
        testKeyPair = KeyPair(privateKey);

        // Create a test script hash
        testScriptHash = UInt160::FromHexString("0x1234567890abcdef1234567890abcdef12345678");
    }
};

TEST_F(WalletAccountTest, DefaultConstructor)
{
    WalletAccount account;

    EXPECT_EQ(UInt160::Zero(), account.GetScriptHash());
    EXPECT_TRUE(account.GetPrivateKey().empty());
    EXPECT_EQ("", account.GetLabel());
    EXPECT_FALSE(account.IsLocked());
    EXPECT_FALSE(account.HasPrivateKey());
}

TEST_F(WalletAccountTest, KeyPairConstructor)
{
    WalletAccount account(testKeyPair);

    EXPECT_NE(UInt160::Zero(), account.GetScriptHash());
    EXPECT_EQ(testKeyPair.GetPublicKey(), account.GetPublicKey());
    EXPECT_EQ(testKeyPair.GetPrivateKey(), account.GetPrivateKey());
    EXPECT_TRUE(account.HasPrivateKey());
    EXPECT_FALSE(account.IsLocked());

    // Contract should be created from public key
    const auto& contract = account.GetContract();
    EXPECT_EQ(account.GetScriptHash(), contract.GetScriptHash());
}

TEST_F(WalletAccountTest, ScriptHashConstructor)
{
    WalletAccount account(testScriptHash);

    EXPECT_EQ(testScriptHash, account.GetScriptHash());
    EXPECT_TRUE(account.GetPrivateKey().empty());
    EXPECT_FALSE(account.HasPrivateKey());
    EXPECT_FALSE(account.IsLocked());
}

TEST_F(WalletAccountTest, GettersAndSetters)
{
    WalletAccount account;

    // Test ScriptHash
    account.SetScriptHash(testScriptHash);
    EXPECT_EQ(testScriptHash, account.GetScriptHash());

    // Test PublicKey
    account.SetPublicKey(testKeyPair.GetPublicKey());
    EXPECT_EQ(testKeyPair.GetPublicKey(), account.GetPublicKey());

    // Test PrivateKey
    std::vector<uint8_t> privateKey = {0x01, 0x02, 0x03};
    account.SetPrivateKey(privateKey);
    EXPECT_EQ(privateKey, account.GetPrivateKey());
    EXPECT_TRUE(account.HasPrivateKey());

    // Test Label
    std::string label = "My Account";
    account.SetLabel(label);
    EXPECT_EQ(label, account.GetLabel());

    // Test Locked
    account.SetLocked(true);
    EXPECT_TRUE(account.IsLocked());
    account.SetLocked(false);
    EXPECT_FALSE(account.IsLocked());
}

TEST_F(WalletAccountTest, Contract)
{
    WalletAccount account(testKeyPair);

    // Create a custom contract
    std::vector<uint8_t> script = {0x21, 0x02};  // Sample script
    std::vector<ContractParameterType> parameterList = {ContractParameterType::Signature};
    Contract customContract(script, parameterList);

    account.SetContract(customContract);
    const auto& retrievedContract = account.GetContract();

    EXPECT_EQ(customContract.GetScript(), retrievedContract.GetScript());
    EXPECT_EQ(customContract.GetParameterList(), retrievedContract.GetParameterList());
}

TEST_F(WalletAccountTest, GetWIF)
{
    WalletAccount account(testKeyPair);

    // WIF should be valid when private key is present
    std::string wif = account.GetWIF();
    EXPECT_FALSE(wif.empty());
    EXPECT_TRUE(wif.length() > 0);

    // WIF should be empty when no private key
    WalletAccount emptyAccount;
    EXPECT_EQ("", emptyAccount.GetWIF());
}

TEST_F(WalletAccountTest, GetAddress)
{
    WalletAccount account(testKeyPair);

    std::string address = account.GetAddress();
    EXPECT_FALSE(address.empty());
    EXPECT_EQ('N', address[0]);  // Neo3 addresses start with 'N'
    EXPECT_GT(address.length(), 20u);

    // Address should be consistent
    EXPECT_EQ(address, account.GetAddress());
}

TEST_F(WalletAccountTest, JsonSerialization)
{
    WalletAccount original(testKeyPair);
    original.SetLabel("Test Account");
    original.SetLocked(true);

    // Serialize to JSON
    nlohmann::json json = original.ToJson();

    // Check JSON contains expected fields
    EXPECT_TRUE(json.contains("scriptHash"));
    EXPECT_TRUE(json.contains("publicKey"));
    EXPECT_TRUE(json.contains("label"));
    EXPECT_TRUE(json.contains("isLocked"));
    EXPECT_TRUE(json.contains("contract"));

    // Deserialize from JSON
    WalletAccount deserialized;
    deserialized.FromJson(json);

    // Compare
    EXPECT_EQ(original.GetScriptHash(), deserialized.GetScriptHash());
    EXPECT_EQ(original.GetPublicKey(), deserialized.GetPublicKey());
    EXPECT_EQ(original.GetLabel(), deserialized.GetLabel());
    EXPECT_EQ(original.IsLocked(), deserialized.IsLocked());
}

TEST_F(WalletAccountTest, JsonSerializationWithoutPrivateKey)
{
    // Create account without private key
    WalletAccount account(testScriptHash);
    account.SetLabel("Watch-only Account");

    // Serialize
    nlohmann::json json = account.ToJson();

    // Should not contain private key
    EXPECT_FALSE(json.contains("privateKey"));
    EXPECT_TRUE(json.contains("scriptHash"));
    EXPECT_TRUE(json.contains("label"));

    // Deserialize
    WalletAccount deserialized;
    deserialized.FromJson(json);

    EXPECT_EQ(account.GetScriptHash(), deserialized.GetScriptHash());
    EXPECT_EQ(account.GetLabel(), deserialized.GetLabel());
    EXPECT_FALSE(deserialized.HasPrivateKey());
}

TEST_F(WalletAccountTest, LockedAccountBehavior)
{
    WalletAccount account(testKeyPair);
    account.SetLocked(true);

    // Locked account should still have access to public information
    EXPECT_FALSE(account.GetAddress().empty());
    EXPECT_NE(UInt160::Zero(), account.GetScriptHash());
    EXPECT_TRUE(account.HasPrivateKey());

    // But WIF might be restricted (implementation dependent)
    // This behavior depends on the specific implementation
}

TEST_F(WalletAccountTest, MultipleAccountsWithSameKeyPair)
{
    WalletAccount account1(testKeyPair);
    WalletAccount account2(testKeyPair);

    // Both accounts should have the same script hash and address
    EXPECT_EQ(account1.GetScriptHash(), account2.GetScriptHash());
    EXPECT_EQ(account1.GetAddress(), account2.GetAddress());
    EXPECT_EQ(account1.GetPublicKey(), account2.GetPublicKey());

    // But they can have different labels
    account1.SetLabel("Account 1");
    account2.SetLabel("Account 2");
    EXPECT_NE(account1.GetLabel(), account2.GetLabel());
}

TEST_F(WalletAccountTest, EmptyPrivateKey)
{
    WalletAccount account;

    // Set empty private key
    account.SetPrivateKey({});
    EXPECT_FALSE(account.HasPrivateKey());
    EXPECT_TRUE(account.GetPrivateKey().empty());
    EXPECT_EQ("", account.GetWIF());
}

TEST_F(WalletAccountTest, ContractUpdate)
{
    WalletAccount account(testKeyPair);

    // Get initial contract
    auto initialContract = account.GetContract();
    auto initialScriptHash = account.GetScriptHash();

    // Create a new contract with different parameters
    std::vector<uint8_t> newScript = {0x51, 0x52, 0x53};  // Different script
    std::vector<ContractParameterType> newParams = {ContractParameterType::Signature, ContractParameterType::Integer};
    Contract newContract(newScript, newParams);

    // Update contract
    account.SetContract(newContract);

    // Script hash should update based on new contract
    EXPECT_NE(initialScriptHash, account.GetScriptHash());
    EXPECT_EQ(newContract.GetScriptHash(), account.GetScriptHash());
}
