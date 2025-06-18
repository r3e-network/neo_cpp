#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>

#include "neo/persistence/storage_key.h"
#include "neo/persistence/storage_item.h"
#include "neo/persistence/data_cache.h"
#include "neo/io/binary_writer.h"
#include "neo/io/binary_reader.h"
#include "neo/io/uint160.h"
#include "neo/io/uint256.h"
#include "tests/utils/test_helpers.h"

using namespace neo::persistence;
using namespace neo::io;
using namespace neo::tests;
using namespace testing;

class Neo3StorageFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Neo N3 uses int32 contract IDs instead of UInt160 script hashes
        test_contract_id_ = 12345;
        test_prefix_ = 0x01;
        test_key_data_ = {0x01, 0x02, 0x03, 0x04};
        test_value_data_ = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    }
    
    int32_t test_contract_id_;
    uint8_t test_prefix_;
    std::vector<uint8_t> test_key_data_;
    std::vector<uint8_t> test_value_data_;
    
    // Helper to create Neo N3 format storage key
    StorageKey CreateNeo3StorageKey(int32_t contract_id, uint8_t prefix, 
                                   const std::vector<uint8_t>& key_data = {}) {
        std::vector<uint8_t> full_key;
        full_key.push_back(prefix);
        full_key.insert(full_key.end(), key_data.begin(), key_data.end());
        return StorageKey(contract_id, full_key);
    }
    
    // Helper to verify Neo N3 serialization format
    bool VerifyNeo3Serialization(const StorageKey& key) {
        BinaryWriter writer;
        key.Serialize(writer);
        auto serialized = writer.ToByteArray();
        
        // Neo N3 format: [contract_id (4 bytes, little-endian)] + [key_data]
        if (serialized.size() < 4) return false;
        
        // Check contract ID is little-endian encoded
        int32_t decoded_id = 
            static_cast<int32_t>(serialized[0]) |
            (static_cast<int32_t>(serialized[1]) << 8) |
            (static_cast<int32_t>(serialized[2]) << 16) |
            (static_cast<int32_t>(serialized[3]) << 24);
        
        return decoded_id == key.GetId();
    }
};

// Test Neo N3 storage key format with contract ID
TEST_F(Neo3StorageFormatTest, StorageKeyWithContractID) {
    auto storage_key = CreateNeo3StorageKey(test_contract_id_, test_prefix_, test_key_data_);
    
    // Verify contract ID is stored correctly
    EXPECT_EQ(storage_key.GetId(), test_contract_id_);
    
    // Verify key data includes prefix
    auto key_data = storage_key.GetKey();
    EXPECT_GE(key_data.size(), 1);
    EXPECT_EQ(key_data[0], test_prefix_);
    
    // Verify full key data
    std::vector<uint8_t> expected_key;
    expected_key.push_back(test_prefix_);
    expected_key.insert(expected_key.end(), test_key_data_.begin(), test_key_data_.end());
    EXPECT_EQ(key_data, expected_key);
}

// Test Neo N3 storage key serialization format
TEST_F(Neo3StorageFormatTest, StorageKeySerialization) {
    auto storage_key = CreateNeo3StorageKey(test_contract_id_, test_prefix_, test_key_data_);
    
    // Test serialization
    BinaryWriter writer;
    storage_key.Serialize(writer);
    auto serialized = writer.ToByteArray();
    
    // Neo N3 format verification
    EXPECT_TRUE(VerifyNeo3Serialization(storage_key));
    
    // Test deserialization
    BinaryReader reader(serialized);
    StorageKey deserialized_key;
    deserialized_key.Deserialize(reader);
    
    EXPECT_EQ(deserialized_key.GetId(), storage_key.GetId());
    EXPECT_EQ(deserialized_key.GetKey(), storage_key.GetKey());
}

// Test factory methods for Neo N3 storage keys
TEST_F(Neo3StorageFormatTest, StorageKeyFactoryMethods) {
    // Test Create with prefix only
    auto key1 = StorageKey::Create(test_contract_id_, test_prefix_);
    EXPECT_EQ(key1.GetId(), test_contract_id_);
    EXPECT_EQ(key1.GetKey().size(), 1);
    EXPECT_EQ(key1.GetKey()[0], test_prefix_);
    
    // Test Create with UInt160
    auto hash160 = TestHelpers::GenerateRandomScriptHash();
    auto key2 = StorageKey::Create(test_contract_id_, test_prefix_, hash160);
    EXPECT_EQ(key2.GetId(), test_contract_id_);
    EXPECT_EQ(key2.GetKey().size(), 1 + 20); // prefix + UInt160
    EXPECT_EQ(key2.GetKey()[0], test_prefix_);
    
    // Test Create with UInt256
    auto hash256 = TestHelpers::GenerateRandomHash();
    auto key3 = StorageKey::Create(test_contract_id_, test_prefix_, hash256);
    EXPECT_EQ(key3.GetId(), test_contract_id_);
    EXPECT_EQ(key3.GetKey().size(), 1 + 32); // prefix + UInt256
    EXPECT_EQ(key3.GetKey()[0], test_prefix_);
    
    // Test Create with custom data
    std::vector<uint8_t> custom_data = {0x11, 0x22, 0x33};
    auto key4 = StorageKey::Create(test_contract_id_, test_prefix_, custom_data);
    EXPECT_EQ(key4.GetId(), test_contract_id_);
    EXPECT_EQ(key4.GetKey().size(), 1 + custom_data.size());
    EXPECT_EQ(key4.GetKey()[0], test_prefix_);
}

// Test Neo N3 native contract storage keys
TEST_F(Neo3StorageFormatTest, NativeContractStorageKeys) {
    // Neo N3 native contract IDs
    const int32_t NEO_TOKEN_ID = -1;
    const int32_t GAS_TOKEN_ID = -2;
    const int32_t POLICY_CONTRACT_ID = -3;
    const int32_t ROLE_MANAGEMENT_ID = -4;
    const int32_t ORACLE_CONTRACT_ID = -5;
    
    // Test NeoToken storage keys
    const uint8_t NEO_COMMITTEE_PREFIX = 14;
    auto neo_committee_key = StorageKey::Create(NEO_TOKEN_ID, NEO_COMMITTEE_PREFIX);
    EXPECT_EQ(neo_committee_key.GetId(), NEO_TOKEN_ID);
    EXPECT_EQ(neo_committee_key.GetKey()[0], NEO_COMMITTEE_PREFIX);
    
    // Test GasToken storage keys
    const uint8_t GAS_ACCOUNT_PREFIX = 20;
    auto account_hash = TestHelpers::GenerateRandomScriptHash();
    auto gas_account_key = StorageKey::Create(GAS_TOKEN_ID, GAS_ACCOUNT_PREFIX, account_hash);
    EXPECT_EQ(gas_account_key.GetId(), GAS_TOKEN_ID);
    EXPECT_EQ(gas_account_key.GetKey()[0], GAS_ACCOUNT_PREFIX);
    
    // Test PolicyContract storage keys
    const uint8_t POLICY_PREFIX = 15;
    auto policy_key = StorageKey::Create(POLICY_CONTRACT_ID, POLICY_PREFIX);
    EXPECT_EQ(policy_key.GetId(), POLICY_CONTRACT_ID);
    EXPECT_EQ(policy_key.GetKey()[0], POLICY_PREFIX);
    
    // Test OracleContract storage keys
    const uint8_t ORACLE_REQUEST_PREFIX = 7;
    auto oracle_key = StorageKey::Create(ORACLE_CONTRACT_ID, ORACLE_REQUEST_PREFIX);
    EXPECT_EQ(oracle_key.GetId(), ORACLE_CONTRACT_ID);
    EXPECT_EQ(oracle_key.GetKey()[0], ORACLE_REQUEST_PREFIX);
}

// Test storage item with interoperable objects
TEST_F(Neo3StorageFormatTest, StorageItemWithInteroperableObjects) {
    StorageItem item;
    
    // Test basic value storage
    item.SetValue(test_value_data_);
    EXPECT_EQ(item.GetValue(), test_value_data_);
    
    // Test interoperable object storage (using UInt256 as example)
    auto hash = TestHelpers::GenerateRandomHash();
    item.SetInteroperable(hash);
    
    auto retrieved_hash = item.GetInteroperable<UInt256>();
    EXPECT_EQ(retrieved_hash, hash);
    
    // Test serialization with interoperable object
    BinaryWriter writer;
    item.Serialize(writer);
    auto serialized = writer.ToByteArray();
    
    BinaryReader reader(serialized);
    StorageItem deserialized_item;
    deserialized_item.Deserialize(reader);
    
    auto final_hash = deserialized_item.GetInteroperable<UInt256>();
    EXPECT_EQ(final_hash, hash);
}

// Test storage key comparison and ordering
TEST_F(Neo3StorageFormatTest, StorageKeyComparison) {
    auto key1 = StorageKey::Create(100, 0x01);
    auto key2 = StorageKey::Create(100, 0x02);
    auto key3 = StorageKey::Create(200, 0x01);
    
    // Test equality
    auto key1_copy = StorageKey::Create(100, 0x01);
    EXPECT_EQ(key1, key1_copy);
    EXPECT_NE(key1, key2);
    EXPECT_NE(key1, key3);
    
    // Test ordering (contract ID first, then key data)
    EXPECT_LT(key1, key3); // Different contract ID
    EXPECT_LT(key1, key2); // Same contract ID, different prefix
    
    // Test hash function for unordered containers
    std::hash<StorageKey> hasher;
    auto hash1 = hasher(key1);
    auto hash2 = hasher(key2);
    auto hash3 = hasher(key3);
    
    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash1, hash3);
    
    // Same key should produce same hash
    auto hash1_copy = hasher(key1_copy);
    EXPECT_EQ(hash1, hash1_copy);
}

// Test data cache with Neo N3 storage format
TEST_F(Neo3StorageFormatTest, DataCacheWithNeo3Format) {
    auto store = std::make_shared<MemoryStore>();
    auto cache = std::make_shared<DataCache>(store);
    
    // Store data using Neo N3 format
    auto key = StorageKey::Create(test_contract_id_, test_prefix_, test_key_data_);
    StorageItem item;
    item.SetValue(test_value_data_);
    
    cache->Add(key, item);
    
    // Retrieve data
    auto retrieved_item = cache->TryGet(key);
    EXPECT_TRUE(retrieved_item.has_value());
    EXPECT_EQ(retrieved_item->GetValue(), test_value_data_);
    
    // Test with different contract ID (should not conflict)
    auto key2 = StorageKey::Create(test_contract_id_ + 1, test_prefix_, test_key_data_);
    StorageItem item2;
    item2.SetValue({0xFF, 0xEE, 0xDD});
    
    cache->Add(key2, item2);
    
    // Both items should exist independently
    auto retrieved1 = cache->TryGet(key);
    auto retrieved2 = cache->TryGet(key2);
    
    EXPECT_TRUE(retrieved1.has_value());
    EXPECT_TRUE(retrieved2.has_value());
    EXPECT_NE(retrieved1->GetValue(), retrieved2->GetValue());
}

// Test migration from Neo 2.x to Neo N3 format
TEST_F(Neo3StorageFormatTest, MigrationFromNeo2xFormat) {
    // Simulate Neo 2.x format (using UInt160 script hash)
    auto script_hash = TestHelpers::GenerateRandomScriptHash();
    
    // Convert to Neo N3 format (contract ID mapping)
    // In real implementation, this would involve contract deployment mapping
    int32_t mapped_contract_id = 42; // Example mapping
    
    // Create Neo N3 storage key
    auto neo3_key = StorageKey::Create(mapped_contract_id, test_prefix_, test_key_data_);
    
    // Verify format compatibility
    EXPECT_EQ(neo3_key.GetId(), mapped_contract_id);
    EXPECT_TRUE(VerifyNeo3Serialization(neo3_key));
    
    // Test that old UInt160-based keys are not compatible
    // (This would be handled by migration logic in real implementation)
}

// Test storage key prefix handling
TEST_F(Neo3StorageFormatTest, StorageKeyPrefixHandling) {
    // Test different prefixes used in Neo N3
    std::vector<uint8_t> neo3_prefixes = {
        0x14, // NEO candidate
        0x0C, // NEO balance
        0x0E, // NEO committee
        0x14, // GAS balance
        0x0F, // Policy contract
        0x09, // Role management
        0x07  // Oracle contract
    };
    
    for (auto prefix : neo3_prefixes) {
        auto key = StorageKey::Create(test_contract_id_, prefix);
        
        EXPECT_EQ(key.GetId(), test_contract_id_);
        EXPECT_EQ(key.GetKey().size(), 1);
        EXPECT_EQ(key.GetKey()[0], prefix);
        
        // Verify serialization
        EXPECT_TRUE(VerifyNeo3Serialization(key));
    }
}

// Test storage with complex key structures
TEST_F(Neo3StorageFormatTest, ComplexKeyStructures) {
    // Test key with multiple components (common in Neo N3)
    std::vector<uint8_t> complex_key;
    complex_key.push_back(0x14); // Prefix
    
    // Add UInt160 (account hash)
    auto account = TestHelpers::GenerateRandomScriptHash();
    auto account_bytes = account.ToArray();
    complex_key.insert(complex_key.end(), account_bytes.begin(), account_bytes.end());
    
    // Add additional data
    std::vector<uint8_t> extra_data = {0x01, 0x02, 0x03};
    complex_key.insert(complex_key.end(), extra_data.begin(), extra_data.end());
    
    auto storage_key = StorageKey(test_contract_id_, complex_key);
    
    // Verify structure
    EXPECT_EQ(storage_key.GetId(), test_contract_id_);
    EXPECT_EQ(storage_key.GetKey().size(), 1 + 20 + 3); // prefix + UInt160 + extra
    EXPECT_EQ(storage_key.GetKey()[0], 0x14);
    
    // Verify serialization
    EXPECT_TRUE(VerifyNeo3Serialization(storage_key));
}

// Test storage enumeration with Neo N3 format
TEST_F(Neo3StorageFormatTest, StorageEnumeration) {
    auto store = std::make_shared<MemoryStore>();
    
    // Add multiple items with same contract ID but different prefixes
    std::vector<std::pair<StorageKey, StorageItem>> test_data;
    
    for (uint8_t prefix = 0x01; prefix <= 0x05; ++prefix) {
        auto key = StorageKey::Create(test_contract_id_, prefix);
        StorageItem item;
        item.SetValue({prefix, prefix, prefix}); // Different values
        
        store->Put(key, item);
        test_data.emplace_back(key, item);
    }
    
    // Test enumeration by contract ID prefix
    auto iterator = store->Find(StorageKey::Create(test_contract_id_, 0x00));
    
    int count = 0;
    while (iterator->Valid()) {
        auto current_key = iterator->Key();
        auto current_value = iterator->Value();
        
        // Verify contract ID matches
        EXPECT_EQ(current_key.GetId(), test_contract_id_);
        
        // Verify key format
        EXPECT_TRUE(VerifyNeo3Serialization(current_key));
        
        iterator->Next();
        count++;
    }
    
    EXPECT_EQ(count, 5); // Should find all 5 items
}

// Test performance with Neo N3 format
TEST_F(Neo3StorageFormatTest, PerformanceWithNeo3Format) {
    auto store = std::make_shared<MemoryStore>();
    
    const int num_items = 10000;
    std::vector<StorageKey> keys;
    std::vector<StorageItem> items;
    
    // Prepare test data
    for (int i = 0; i < num_items; ++i) {
        auto key = StorageKey::Create(test_contract_id_, 0x01, 
                                    TestHelpers::GenerateRandomBytes(20));
        StorageItem item;
        item.SetValue(TestHelpers::GenerateRandomBytes(100));
        
        keys.push_back(key);
        items.push_back(item);
    }
    
    // Measure insertion performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_items; ++i) {
        store->Put(keys[i], items[i]);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should handle insertions efficiently
    double ms_per_item = static_cast<double>(duration.count()) / num_items;
    EXPECT_LT(ms_per_item, 1.0); // Less than 1ms per item
    
    // Measure retrieval performance
    start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_items; ++i) {
        auto retrieved = store->TryGet(keys[i]);
        EXPECT_TRUE(retrieved.has_value());
    }
    
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    ms_per_item = static_cast<double>(duration.count()) / num_items;
    EXPECT_LT(ms_per_item, 0.5); // Less than 0.5ms per retrieval
}

// Test edge cases and error handling
TEST_F(Neo3StorageFormatTest, EdgeCasesAndErrorHandling) {
    // Test with negative contract ID
    auto key1 = StorageKey::Create(-1, 0x01); // Native contract
    EXPECT_EQ(key1.GetId(), -1);
    EXPECT_TRUE(VerifyNeo3Serialization(key1));
    
    // Test with zero contract ID
    auto key2 = StorageKey::Create(0, 0x01);
    EXPECT_EQ(key2.GetId(), 0);
    EXPECT_TRUE(VerifyNeo3Serialization(key2));
    
    // Test with maximum contract ID
    auto key3 = StorageKey::Create(INT32_MAX, 0x01);
    EXPECT_EQ(key3.GetId(), INT32_MAX);
    EXPECT_TRUE(VerifyNeo3Serialization(key3));
    
    // Test with empty key data
    auto key4 = StorageKey(test_contract_id_, {});
    EXPECT_EQ(key4.GetId(), test_contract_id_);
    EXPECT_TRUE(key4.GetKey().empty());
    EXPECT_TRUE(VerifyNeo3Serialization(key4));
    
    // Test with large key data
    std::vector<uint8_t> large_key_data(1000, 0xAA);
    auto key5 = StorageKey(test_contract_id_, large_key_data);
    EXPECT_EQ(key5.GetId(), test_contract_id_);
    EXPECT_EQ(key5.GetKey().size(), 1000);
    EXPECT_TRUE(VerifyNeo3Serialization(key5));
}