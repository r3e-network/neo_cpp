#include <neo/wallets/wallet_transaction.h>
#include <neo/network/p2p/payloads/transaction.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <gtest/gtest.h>
#include <sstream>
#include <memory>

using namespace neo::wallets;
using namespace neo::network::p2p::payloads;
using namespace neo::io;

class MockTransaction : public Transaction
{
public:
    MockTransaction() : Transaction() {}
    
    UInt256 GetHash() const override
    {
        return UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    }
    
    void Serialize(BinaryWriter& writer) const override
    {
        // Simple serialization for testing
        writer.Write(static_cast<uint8_t>(0x01));
    }
    
    void Deserialize(BinaryReader& reader) override
    {
        // Simple deserialization for testing
        reader.ReadUInt8();
    }
};

class UT_WalletTransaction : public testing::Test
{
protected:
    void SetUp() override
    {
        // Create a mock transaction
        transaction = std::make_shared<MockTransaction>();
    }
    
    std::shared_ptr<Transaction> transaction;
};

TEST_F(UT_WalletTransaction, TestConstructor)
{
    // Default constructor
    WalletTransaction tx1;
    EXPECT_EQ(UInt256(), tx1.GetHash());
    EXPECT_EQ(nullptr, tx1.GetTransaction());
    EXPECT_EQ(0, tx1.GetHeight());
    
    // Constructor with transaction
    WalletTransaction tx2(*transaction);
    EXPECT_EQ(transaction->GetHash(), tx2.GetHash());
    EXPECT_NE(nullptr, tx2.GetTransaction());
    EXPECT_EQ(0, tx2.GetHeight());
    
    // Constructor with transaction and height
    WalletTransaction tx3(*transaction, 123);
    EXPECT_EQ(transaction->GetHash(), tx3.GetHash());
    EXPECT_NE(nullptr, tx3.GetTransaction());
    EXPECT_EQ(123, tx3.GetHeight());
}

TEST_F(UT_WalletTransaction, TestGettersAndSetters)
{
    WalletTransaction tx;
    
    // Test hash getter and setter
    UInt256 hash = UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    tx.SetHash(hash);
    EXPECT_EQ(hash, tx.GetHash());
    
    // Test transaction getter and setter
    tx.SetTransaction(transaction);
    EXPECT_EQ(transaction, tx.GetTransaction());
    EXPECT_EQ(transaction->GetHash(), tx.GetHash()); // Hash should be updated
    
    // Test height getter and setter
    tx.SetHeight(456);
    EXPECT_EQ(456, tx.GetHeight());
    
    // Test time getter and setter
    auto now = std::chrono::system_clock::now();
    tx.SetTime(now);
    EXPECT_EQ(now, tx.GetTime());
}

TEST_F(UT_WalletTransaction, TestJsonSerialization)
{
    // Create a wallet transaction
    WalletTransaction tx(*transaction, 789);
    auto originalTime = tx.GetTime();
    
    // Serialize to JSON
    std::stringstream stream;
    JsonWriter writer(stream);
    tx.SerializeJson(writer);
    
    // Deserialize from JSON
    JsonReader reader(stream.str());
    WalletTransaction deserializedTx;
    deserializedTx.DeserializeJson(reader);
    
    // Verify deserialized values
    EXPECT_EQ(tx.GetHash(), deserializedTx.GetHash());
    EXPECT_EQ(tx.GetHeight(), deserializedTx.GetHeight());
    
    // Time should be approximately equal (may lose some precision due to millisecond conversion)
    auto originalMillis = std::chrono::duration_cast<std::chrono::milliseconds>(originalTime.time_since_epoch()).count();
    auto deserializedMillis = std::chrono::duration_cast<std::chrono::milliseconds>(deserializedTx.GetTime().time_since_epoch()).count();
    EXPECT_EQ(originalMillis, deserializedMillis);
    
    // Transaction should be deserialized
    EXPECT_NE(nullptr, deserializedTx.GetTransaction());
}
