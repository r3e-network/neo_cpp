#include <gtest/gtest.h>
#include <memory>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/memory_stream.h>
#include <neo/io/uint160.h>
#include <neo/ledger/signer.h>
#include <string>
#include <vector>

using namespace neo::ledger;
using namespace neo::cryptography::ecc;
using namespace neo::io;

/**
 * @brief Test fixture for Signer
 */
class SignerTest : public testing::Test
{
  protected:
    UInt160 testAccount;
    WitnessScope testScopes;
    std::vector<UInt160> testAllowedContracts;
    std::vector<ECPoint> testAllowedGroups;

    void SetUp() override
    {
        // Initialize test data
        testAccount = UInt160::FromHexString("0x1234567890abcdef1234567890abcdef12345678");
        testScopes = WitnessScope::CalledByEntry;

        // Create test allowed contracts
        testAllowedContracts.push_back(UInt160::FromHexString("0xabcdef0123456789abcdef0123456789abcdef01"));
        testAllowedContracts.push_back(UInt160::FromHexString("0x9876543210fedcba9876543210fedcba98765432"));

        // Create test allowed groups (mock EC points)
        ByteVector pubKey1(33, 0x02);  // Compressed public key starting with 0x02
        ByteVector pubKey2(33, 0x03);  // Compressed public key starting with 0x03
        testAllowedGroups.push_back(ECPoint::DecodePoint(pubKey1));
        testAllowedGroups.push_back(ECPoint::DecodePoint(pubKey2));
    }
};

TEST_F(SignerTest, DefaultConstructor)
{
    Signer signer;

    EXPECT_EQ(UInt160::Zero(), signer.GetAccount());
    EXPECT_EQ(WitnessScope::None, signer.GetScopes());
    EXPECT_TRUE(signer.GetAllowedContracts().empty());
    EXPECT_TRUE(signer.GetAllowedGroups().empty());
}

TEST_F(SignerTest, ParameterizedConstructor)
{
    Signer signer(testAccount, testScopes);

    EXPECT_EQ(testAccount, signer.GetAccount());
    EXPECT_EQ(testScopes, signer.GetScopes());
    EXPECT_TRUE(signer.GetAllowedContracts().empty());
    EXPECT_TRUE(signer.GetAllowedGroups().empty());
}

TEST_F(SignerTest, GettersAndSetters)
{
    Signer signer;

    // Test Account
    signer.SetAccount(testAccount);
    EXPECT_EQ(testAccount, signer.GetAccount());

    // Test Scopes
    signer.SetScopes(testScopes);
    EXPECT_EQ(testScopes, signer.GetScopes());

    // Test AllowedContracts
    signer.SetAllowedContracts(testAllowedContracts);
    EXPECT_EQ(testAllowedContracts, signer.GetAllowedContracts());
    EXPECT_EQ(2u, signer.GetAllowedContracts().size());

    // Test AllowedGroups
    signer.SetAllowedGroups(testAllowedGroups);
    EXPECT_EQ(testAllowedGroups, signer.GetAllowedGroups());
    EXPECT_EQ(2u, signer.GetAllowedGroups().size());
}

TEST_F(SignerTest, WitnessScopeValues)
{
    // Test all witness scope values
    EXPECT_EQ(0x00, static_cast<uint8_t>(WitnessScope::None));
    EXPECT_EQ(0x01, static_cast<uint8_t>(WitnessScope::CalledByEntry));
    EXPECT_EQ(0x10, static_cast<uint8_t>(WitnessScope::CustomContracts));
    EXPECT_EQ(0x20, static_cast<uint8_t>(WitnessScope::CustomGroups));
    EXPECT_EQ(0x40, static_cast<uint8_t>(WitnessScope::WitnessRules));
    EXPECT_EQ(0x80, static_cast<uint8_t>(WitnessScope::Global));
}

TEST_F(SignerTest, Serialization)
{
    Signer original(testAccount, WitnessScope::CustomContracts);
    original.SetAllowedContracts(testAllowedContracts);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    Signer deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetAccount(), deserialized.GetAccount());
    EXPECT_EQ(original.GetScopes(), deserialized.GetScopes());
    EXPECT_EQ(original.GetAllowedContracts(), deserialized.GetAllowedContracts());
}

TEST_F(SignerTest, JsonSerialization)
{
    Signer original(testAccount, testScopes);

    // Serialize to JSON
    JsonWriter writer;
    original.SerializeJson(writer);
    std::string json = writer.ToString();

    // Deserialize from JSON
    JsonReader reader(json);
    Signer deserialized;
    deserialized.DeserializeJson(reader);

    // Compare
    EXPECT_EQ(original.GetAccount(), deserialized.GetAccount());
    EXPECT_EQ(original.GetScopes(), deserialized.GetScopes());
}

TEST_F(SignerTest, EqualityOperator)
{
    Signer signer1(testAccount, testScopes);
    Signer signer2(testAccount, testScopes);
    Signer signer3(UInt160::Zero(), testScopes);

    // Same signers
    EXPECT_TRUE(signer1 == signer2);
    EXPECT_FALSE(signer1 != signer2);

    // Different account
    EXPECT_FALSE(signer1 == signer3);
    EXPECT_TRUE(signer1 != signer3);

    // Different scopes
    signer2.SetScopes(WitnessScope::Global);
    EXPECT_FALSE(signer1 == signer2);
    EXPECT_TRUE(signer1 != signer2);
}

TEST_F(SignerTest, ComplexScopes)
{
    // Test combining multiple scopes
    WitnessScope combinedScopes = static_cast<WitnessScope>(static_cast<uint8_t>(WitnessScope::CalledByEntry) |
                                                            static_cast<uint8_t>(WitnessScope::CustomContracts) |
                                                            static_cast<uint8_t>(WitnessScope::CustomGroups));

    Signer signer(testAccount, combinedScopes);
    signer.SetAllowedContracts(testAllowedContracts);
    signer.SetAllowedGroups(testAllowedGroups);

    EXPECT_EQ(combinedScopes, signer.GetScopes());
    EXPECT_EQ(testAllowedContracts, signer.GetAllowedContracts());
    EXPECT_EQ(testAllowedGroups, signer.GetAllowedGroups());
}

TEST_F(SignerTest, GlobalScope)
{
    Signer signer(testAccount, WitnessScope::Global);

    // With global scope, allowed contracts and groups should be ignored
    signer.SetAllowedContracts(testAllowedContracts);
    signer.SetAllowedGroups(testAllowedGroups);

    EXPECT_EQ(WitnessScope::Global, signer.GetScopes());
    // Note: In practice, these might be cleared when Global is set
    EXPECT_EQ(testAllowedContracts, signer.GetAllowedContracts());
    EXPECT_EQ(testAllowedGroups, signer.GetAllowedGroups());
}

TEST_F(SignerTest, EmptyAllowedLists)
{
    Signer signer(testAccount, WitnessScope::CustomContracts);

    // Set empty lists
    std::vector<UInt160> emptyContracts;
    std::vector<ECPoint> emptyGroups;

    signer.SetAllowedContracts(emptyContracts);
    signer.SetAllowedGroups(emptyGroups);

    EXPECT_TRUE(signer.GetAllowedContracts().empty());
    EXPECT_TRUE(signer.GetAllowedGroups().empty());
}

TEST_F(SignerTest, LargeAllowedLists)
{
    Signer signer(testAccount, WitnessScope::CustomContracts);

    // Create large lists
    std::vector<UInt160> manyContracts;
    for (int i = 0; i < 100; ++i)
    {
        std::string hashStr = "0x";
        for (int j = 0; j < 40; ++j)
        {
            hashStr += std::to_string((i + j) % 10);
        }
        manyContracts.push_back(UInt160::FromHexString(hashStr));
    }

    signer.SetAllowedContracts(manyContracts);
    EXPECT_EQ(100u, signer.GetAllowedContracts().size());
}

TEST_F(SignerTest, SerializationWithGroups)
{
    WitnessScope scopeWithGroups = static_cast<WitnessScope>(static_cast<uint8_t>(WitnessScope::CalledByEntry) |
                                                             static_cast<uint8_t>(WitnessScope::CustomGroups));

    Signer original(testAccount, scopeWithGroups);
    original.SetAllowedGroups(testAllowedGroups);

    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);

    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    Signer deserialized;
    deserialized.Deserialize(reader);

    // Compare
    EXPECT_EQ(original.GetAccount(), deserialized.GetAccount());
    EXPECT_EQ(original.GetScopes(), deserialized.GetScopes());
    EXPECT_EQ(original.GetAllowedGroups().size(), deserialized.GetAllowedGroups().size());
}

TEST_F(SignerTest, UpdateAfterConstruction)
{
    Signer signer(testAccount, WitnessScope::None);

    // Update all fields
    UInt160 newAccount = UInt160::FromHexString("0xfedcba0987654321fedcba0987654321fedcba09");
    WitnessScope newScopes = WitnessScope::Global;

    signer.SetAccount(newAccount);
    signer.SetScopes(newScopes);

    EXPECT_EQ(newAccount, signer.GetAccount());
    EXPECT_EQ(newScopes, signer.GetScopes());
}

TEST_F(SignerTest, MultipleSigner)
{
    // Test creating multiple signers for a transaction
    std::vector<Signer> signers;

    // Primary signer with global scope
    signers.emplace_back(testAccount, WitnessScope::Global);

    // Secondary signer with limited scope
    UInt160 account2 = UInt160::FromHexString("0xaabbccddeeff00112233445566778899aabbccdd");
    signers.emplace_back(account2, WitnessScope::CalledByEntry);

    // Third signer with custom contracts
    UInt160 account3 = UInt160::FromHexString("0x1122334455667788990011223344556677889900");
    Signer signer3(account3, WitnessScope::CustomContracts);
    signer3.SetAllowedContracts(testAllowedContracts);
    signers.push_back(signer3);

    EXPECT_EQ(3u, signers.size());
    EXPECT_EQ(WitnessScope::Global, signers[0].GetScopes());
    EXPECT_EQ(WitnessScope::CalledByEntry, signers[1].GetScopes());
    EXPECT_EQ(WitnessScope::CustomContracts, signers[2].GetScopes());
    EXPECT_EQ(2u, signers[2].GetAllowedContracts().size());
}
