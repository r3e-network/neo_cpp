#include <gtest/gtest.h>
#include <neo/cryptography/mpttrie/trie.h>
#include <neo/cryptography/mpttrie/node.h>
#include <neo/cryptography/mpttrie/cache.h>
#include <neo/persistence/memory_store.h>
#include <memory>

namespace neo::cryptography::mpttrie::tests
{
    class MPTTrieTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            store = std::make_shared<persistence::MemoryStore>();
            snapshot = store->GetSnapshot();
        }

        std::shared_ptr<persistence::IStore> store;
        std::shared_ptr<persistence::IStoreSnapshot> snapshot;
    };

    TEST_F(MPTTrieTest, TestNodeCreation)
    {
        // Test creating different node types
        auto leaf_node = Node::NewLeaf(std::vector<uint8_t>{0x01, 0x02, 0x03});
        EXPECT_EQ(NodeType::LeafNode, leaf_node->GetNodeType());
        EXPECT_FALSE(leaf_node->IsEmpty());

        auto empty_node = Node::NewEmpty();
        EXPECT_EQ(NodeType::EmptyNode, empty_node->GetNodeType());
        EXPECT_TRUE(empty_node->IsEmpty());

        auto hash = io::UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
        auto hash_node = Node::NewHash(hash);
        EXPECT_EQ(NodeType::HashNode, hash_node->GetNodeType());
        EXPECT_FALSE(hash_node->IsEmpty());
    }

    TEST_F(MPTTrieTest, TestNodeSerialization)
    {
        // Test leaf node serialization
        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04};
        auto leaf_node = Node::NewLeaf(data);

        auto serialized = leaf_node->Encode();
        EXPECT_FALSE(serialized.empty());

        auto deserialized = Node::Decode(serialized);
        EXPECT_EQ(leaf_node->GetNodeType(), deserialized->GetNodeType());
        EXPECT_EQ(leaf_node->IsEmpty(), deserialized->IsEmpty());
    }

    TEST_F(MPTTrieTest, TestTrieCreation)
    {
        // Test creating empty trie
        Trie trie(snapshot, io::UInt256::Zero(), true);
        EXPECT_TRUE(trie.GetRoot().IsEmpty());

        // Test creating trie with root hash
        auto hash = io::UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
        Trie trie_with_root(snapshot, hash, true);
        EXPECT_FALSE(trie_with_root.GetRoot().IsEmpty());
    }

    TEST_F(MPTTrieTest, TestTriePutAndGet)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        // Test putting and getting a value
        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> value = {0x04, 0x05, 0x06};

        trie.Put(key, value);

        auto retrieved = trie.Get(key);
        EXPECT_EQ(value, retrieved);
    }

    TEST_F(MPTTrieTest, TestTrieTryGet)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> value = {0x04, 0x05, 0x06};

        // Test getting non-existent key
        io::ByteVector result;
        bool found = trie.TryGet(key, result);
        EXPECT_FALSE(found);

        // Put value and try again
        trie.Put(key, value);
        found = trie.TryGet(key, result);
        EXPECT_TRUE(found);
        EXPECT_EQ(value.size(), result.Size());

        for (size_t i = 0; i < value.size(); ++i)
        {
            EXPECT_EQ(value[i], result[i]);
        }
    }

    TEST_F(MPTTrieTest, TestTrieDelete)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> value = {0x04, 0x05, 0x06};

        // Put value
        trie.Put(key, value);

        // Verify it exists
        io::ByteVector result;
        bool found = trie.TryGet(key, result);
        EXPECT_TRUE(found);

        // Delete it
        bool deleted = trie.Delete(key);
        EXPECT_TRUE(deleted);

        // Verify it's gone
        found = trie.TryGet(key, result);
        EXPECT_FALSE(found);

        // Try to delete non-existent key
        deleted = trie.Delete(key);
        EXPECT_FALSE(deleted);
    }

    TEST_F(MPTTrieTest, TestTrieMultipleKeys)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        // Test multiple key-value pairs
        std::vector<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> test_data = {
            {{0x01}, {0x11}},
            {{0x01, 0x02}, {0x12}},
            {{0x01, 0x02, 0x03}, {0x13}},
            {{0x02}, {0x21}},
            {{0x02, 0x03}, {0x23}}
        };

        // Put all values
        for (const auto& [key, value] : test_data)
        {
            trie.Put(key, value);
        }

        // Verify all values can be retrieved
        for (const auto& [key, expected_value] : test_data)
        {
            auto retrieved = trie.Get(key);
            EXPECT_EQ(expected_value, retrieved);
        }
    }

    TEST_F(MPTTrieTest, TestTrieProof)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> value = {0x04, 0x05, 0x06};

        trie.Put(key, value);

        // Test getting proof for existing key
        std::unordered_set<io::ByteVector> proof;
        bool has_proof = trie.TryGetProof(key, proof);
        EXPECT_TRUE(has_proof);
        EXPECT_FALSE(proof.empty());

        // Test getting proof for non-existent key
        std::vector<uint8_t> non_existent_key = {0x99, 0x99, 0x99};
        std::unordered_set<io::ByteVector> empty_proof;
        bool no_proof = trie.TryGetProof(non_existent_key, empty_proof);
        EXPECT_FALSE(no_proof);
    }

    TEST_F(MPTTrieTest, TestTrieCommit)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> value = {0x04, 0x05, 0x06};

        trie.Put(key, value);

        // Commit should not throw
        EXPECT_NO_THROW(trie.Commit());
    }

    TEST_F(MPTTrieTest, TestNibblesConversion)
    {
        std::vector<uint8_t> input = {0x12, 0x34, 0x56};
        auto nibbles = Trie::ToNibbles(input);

        // Each byte should become two nibbles
        EXPECT_EQ(6, nibbles.size());
        EXPECT_EQ(0x01, nibbles[0]);
        EXPECT_EQ(0x02, nibbles[1]);
        EXPECT_EQ(0x03, nibbles[2]);
        EXPECT_EQ(0x04, nibbles[3]);
        EXPECT_EQ(0x05, nibbles[4]);
        EXPECT_EQ(0x06, nibbles[5]);
    }

    TEST_F(MPTTrieTest, TestKeyCreation)
    {
        auto hash = io::UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
        auto key = Trie::CreateKey(hash);

        EXPECT_EQ(io::UInt256::Size + 1, key.Size());
        EXPECT_EQ(Trie::Prefix, key[0]);

        // Verify hash bytes are correctly copied
        for (size_t i = 0; i < io::UInt256::Size; ++i)
        {
            EXPECT_EQ(hash.Data()[i], key[i + 1]);
        }
    }

    TEST_F(MPTTrieTest, TestCommonPrefix)
    {
        std::vector<uint8_t> a = {0x01, 0x02, 0x03, 0x04};
        std::vector<uint8_t> b = {0x01, 0x02, 0x05, 0x06};

        auto common = Trie::CommonPrefix(a, b);
        EXPECT_EQ(2, common.size());
        EXPECT_EQ(0x01, common[0]);
        EXPECT_EQ(0x02, common[1]);

        // Test with no common prefix
        std::vector<uint8_t> c = {0x07, 0x08};
        auto no_common = Trie::CommonPrefix(a, c);
        EXPECT_EQ(0, no_common.size());

        // Test with identical arrays
        auto all_common = Trie::CommonPrefix(a, a);
        EXPECT_EQ(a.size(), all_common.size());
    }

    TEST_F(MPTTrieTest, TestCacheOperations)
    {
        Cache cache(snapshot, Trie::Prefix);

        // Test resolving non-existent hash
        auto hash = io::UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
        auto node = cache.Resolve(hash);
        EXPECT_NE(nullptr, node);

        // Test putting node
        auto leaf_node = Node::NewLeaf(std::vector<uint8_t>{0x01, 0x02, 0x03});
        EXPECT_NO_THROW(cache.PutNode(std::move(leaf_node)));

        // Test deleting node
        EXPECT_NO_THROW(cache.DeleteNode(hash));

        // Test commit
        EXPECT_NO_THROW(cache.Commit());
    }

    TEST_F(MPTTrieTest, TestErrorConditions)
    {
        Trie trie(snapshot, io::UInt256::Zero(), true);

        // Test with empty key
        std::vector<uint8_t> empty_key;
        std::vector<uint8_t> value = {0x01, 0x02, 0x03};

        EXPECT_THROW(trie.Put(empty_key, value), std::invalid_argument);

        // Test with oversized key
        std::vector<uint8_t> oversized_key(Node::MaxKeyLength + 1, 0x01);
        EXPECT_THROW(trie.Put(oversized_key, value), std::invalid_argument);

        // Test with oversized value
        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::vector<uint8_t> oversized_value(Node::MaxValueLength + 1, 0x01);
        EXPECT_THROW(trie.Put(key, oversized_value), std::invalid_argument);
    }

    TEST_F(MPTTrieTest, TestVerifyProof)
    {
        // Test static proof verification method
        auto root = io::UInt256::Parse("0x1234567890123456789012345678901234567890123456789012345678901234");
        std::vector<uint8_t> key = {0x01, 0x02, 0x03};
        std::unordered_set<io::ByteVector> proof;

        // Test with empty proof - should return empty result
        auto result = Trie::VerifyProof(root, key, proof);
        EXPECT_TRUE(result.empty()); // Empty proof should return empty result
    }

    TEST_F(MPTTrieTest, TestNodeConstants)
    {
        // Test that node constants are reasonable
        EXPECT_GT(Node::MaxKeyLength, 0);
        EXPECT_GT(Node::MaxValueLength, 0);
        EXPECT_LT(Node::MaxKeyLength, 1000000); // Sanity check
        EXPECT_LT(Node::MaxValueLength, 1000000); // Sanity check
    }
}
