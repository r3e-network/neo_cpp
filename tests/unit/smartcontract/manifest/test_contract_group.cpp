#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/smartcontract/manifest/contract_group.h"
#include "neo/cryptography/ecc/ecpoint.h"
#include "neo/cryptography/ecc/keypair.h"
#include "neo/cryptography/ecdsa.h"
#include "neo/vm/stack_item.h"
#include "neo/io/uint160.h"

using namespace neo::smartcontract::manifest;
using namespace neo::cryptography::ecc;
using namespace neo::cryptography;
using namespace neo::vm;
using namespace neo::io;

class ContractGroupTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate test key pair
        test_keypair_ = KeyPair::Generate();
        test_public_key_ = test_keypair_.GetPublicKey();
        
        // Test contract hash
        test_contract_hash_ = UInt160::Parse("0x1234567890123456789012345678901234567890");
        
        // Create test signature
        std::vector<uint8_t> message_to_sign = test_contract_hash_.GetBytes();
        test_signature_ = ECDSA::Sign(message_to_sign, test_keypair_.GetPrivateKey());
    }

    KeyPair test_keypair_;
    ECPoint test_public_key_;
    UInt160 test_contract_hash_;
    std::vector<uint8_t> test_signature_;
};

TEST_F(ContractGroupTest, ConstructorAndGetters) {
    ContractGroup group(test_public_key_, test_signature_);
    
    EXPECT_EQ(group.GetPublicKey(), test_public_key_);
    EXPECT_EQ(group.GetSignature(), test_signature_);
}

TEST_F(ContractGroupTest, TestClone) {
    ContractGroup original(test_public_key_, test_signature_);
    
    // Convert to stack item
    auto stack_item = original.ToStackItem();
    ASSERT_NE(stack_item, nullptr);
    
    // Create new group from stack item (equivalent to cloning)
    auto cloned = ContractGroup::FromStackItem(stack_item);
    ASSERT_NE(cloned, nullptr);
    
    EXPECT_EQ(original.GetPublicKey(), cloned->GetPublicKey());
    EXPECT_EQ(original.GetSignature(), cloned->GetSignature());
}

TEST_F(ContractGroupTest, TestIsValid_ValidSignature) {
    ContractGroup group(test_public_key_, test_signature_);
    
    // Should be valid since we created signature with correct key and hash
    EXPECT_TRUE(group.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, TestIsValid_InvalidSignature) {
    // Create invalid signature (wrong message)
    std::vector<uint8_t> wrong_message = {0x01, 0x02, 0x03, 0x04};
    auto wrong_signature = ECDSA::Sign(wrong_message, test_keypair_.GetPrivateKey());
    
    ContractGroup group(test_public_key_, wrong_signature);
    
    // Should be invalid since signature was created for different message
    EXPECT_FALSE(group.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, TestIsValid_WrongPublicKey) {
    // Generate different key pair
    auto wrong_keypair = KeyPair::Generate();
    auto wrong_public_key = wrong_keypair.GetPublicKey();
    
    ContractGroup group(wrong_public_key, test_signature_);
    
    // Should be invalid since signature was created with different key
    EXPECT_FALSE(group.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, Serialization) {
    ContractGroup original(test_public_key_, test_signature_);
    
    // Serialize
    MemoryStream stream;
    BinaryWriter writer(stream);
    original.Serialize(writer);
    
    // Deserialize
    stream.Seek(0, SeekOrigin::Begin);
    BinaryReader reader(stream);
    ContractGroup deserialized;
    deserialized.Deserialize(reader);
    
    EXPECT_EQ(original.GetPublicKey(), deserialized.GetPublicKey());
    EXPECT_EQ(original.GetSignature(), deserialized.GetSignature());
}

TEST_F(ContractGroupTest, JsonSerialization) {
    ContractGroup group(test_public_key_, test_signature_);
    
    // Convert to JSON
    auto json_str = group.ToJson();
    EXPECT_FALSE(json_str.empty());
    
    // Parse back from JSON
    auto parsed_group = ContractGroup::FromJson(json_str);
    ASSERT_NE(parsed_group, nullptr);
    
    EXPECT_EQ(group.GetPublicKey(), parsed_group->GetPublicKey());
    EXPECT_EQ(group.GetSignature(), parsed_group->GetSignature());
}

TEST_F(ContractGroupTest, EqualityOperator) {
    ContractGroup group1(test_public_key_, test_signature_);
    ContractGroup group2(test_public_key_, test_signature_);
    
    // Generate different group
    auto different_keypair = KeyPair::Generate();
    auto different_message = UInt160::Parse("0x9876543210987654321098765432109876543210").GetBytes();
    auto different_signature = ECDSA::Sign(different_message, different_keypair.GetPrivateKey());
    ContractGroup group3(different_keypair.GetPublicKey(), different_signature);
    
    EXPECT_EQ(group1, group2);
    EXPECT_NE(group1, group3);
}

TEST_F(ContractGroupTest, EmptySignature) {
    std::vector<uint8_t> empty_signature;
    ContractGroup group(test_public_key_, empty_signature);
    
    // Should be invalid with empty signature
    EXPECT_FALSE(group.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, MalformedSignature) {
    // Create signature with wrong length
    std::vector<uint8_t> malformed_signature(30, 0xFF); // Wrong length
    ContractGroup group(test_public_key_, malformed_signature);
    
    // Should be invalid with malformed signature
    EXPECT_FALSE(group.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, GetSize) {
    ContractGroup group(test_public_key_, test_signature_);
    
    size_t expected_size = 33 + test_signature_.size(); // 33 bytes for compressed public key + signature
    EXPECT_EQ(group.GetSize(), expected_size);
}

TEST_F(ContractGroupTest, MultipleContractHashes) {
    ContractGroup group(test_public_key_, test_signature_);
    
    // Test with the correct hash
    EXPECT_TRUE(group.IsValid(test_contract_hash_));
    
    // Test with different hashes
    auto different_hash1 = UInt160::Parse("0x9876543210987654321098765432109876543210");
    auto different_hash2 = UInt160::Parse("0xabcdefabcdefabcdefabcdefabcdefabcdefabcd");
    
    EXPECT_FALSE(group.IsValid(different_hash1));
    EXPECT_FALSE(group.IsValid(different_hash2));
}

TEST_F(ContractGroupTest, SignatureVerificationEdgeCases) {
    ContractGroup group(test_public_key_, test_signature_);
    
    // Test with zero hash
    auto zero_hash = UInt160::Zero();
    EXPECT_FALSE(group.IsValid(zero_hash));
    
    // Test with max hash
    std::vector<uint8_t> max_bytes(20, 0xFF);
    auto max_hash = UInt160(max_bytes);
    EXPECT_FALSE(group.IsValid(max_hash));
}

TEST_F(ContractGroupTest, CopyConstructor) {
    ContractGroup original(test_public_key_, test_signature_);
    ContractGroup copy(original);
    
    EXPECT_EQ(original.GetPublicKey(), copy.GetPublicKey());
    EXPECT_EQ(original.GetSignature(), copy.GetSignature());
    EXPECT_EQ(original.IsValid(test_contract_hash_), copy.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, AssignmentOperator) {
    ContractGroup original(test_public_key_, test_signature_);
    ContractGroup assigned;
    
    assigned = original;
    
    EXPECT_EQ(original.GetPublicKey(), assigned.GetPublicKey());
    EXPECT_EQ(original.GetSignature(), assigned.GetSignature());
    EXPECT_EQ(original.IsValid(test_contract_hash_), assigned.IsValid(test_contract_hash_));
}

TEST_F(ContractGroupTest, StackItemConversion) {
    ContractGroup original(test_public_key_, test_signature_);
    
    // Convert to stack item
    auto stack_item = original.ToStackItem();
    ASSERT_NE(stack_item, nullptr);
    
    // Verify stack item structure (should be a struct with 2 elements: pubkey and signature)
    auto struct_item = std::dynamic_pointer_cast<StructStackItem>(stack_item);
    ASSERT_NE(struct_item, nullptr);
    EXPECT_EQ(struct_item->GetCount(), 2);
    
    // Convert back from stack item
    auto reconstructed = ContractGroup::FromStackItem(stack_item);
    ASSERT_NE(reconstructed, nullptr);
    
    EXPECT_EQ(original.GetPublicKey(), reconstructed->GetPublicKey());
    EXPECT_EQ(original.GetSignature(), reconstructed->GetSignature());
}

TEST_F(ContractGroupTest, InvalidStackItemConversion) {
    // Test with invalid stack items
    auto invalid_item1 = std::make_shared<IntegerStackItem>(42);
    auto invalid_item2 = std::make_shared<StructStackItem>();
    
    EXPECT_THROW(ContractGroup::FromStackItem(invalid_item1), std::exception);
    EXPECT_THROW(ContractGroup::FromStackItem(invalid_item2), std::exception);
}

TEST_F(ContractGroupTest, SignatureRoundTrip) {
    // Test complete signature creation and verification cycle
    auto keypair = KeyPair::Generate();
    auto contract_hash = UInt160::Parse("0xfedcba0987654321fedcba0987654321fedcba09");
    
    // Create signature for the contract hash
    auto message = contract_hash.GetBytes();
    auto signature = ECDSA::Sign(message, keypair.GetPrivateKey());
    
    // Create group and verify
    ContractGroup group(keypair.GetPublicKey(), signature);
    EXPECT_TRUE(group.IsValid(contract_hash));
    
    // Serialize and deserialize
    auto json_str = group.ToJson();
    auto restored_group = ContractGroup::FromJson(json_str);
    ASSERT_NE(restored_group, nullptr);
    
    // Verify signature still works after round trip
    EXPECT_TRUE(restored_group->IsValid(contract_hash));
}