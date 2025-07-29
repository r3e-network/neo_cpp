#include <gtest/gtest.h>
#include <neo/cryptography/ecc/secp256r1.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>
#include <neo/node/node.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
#include <sstream>

using namespace neo::node;
using namespace neo::persistence;
using namespace neo::ledger;
using namespace neo::io;
using namespace neo::cryptography::ecc;

class NodeTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Create store provider
        auto store = std::make_shared<MemoryStore>();
        storeProvider_ = std::make_shared<StoreProvider>(store);

        // Create settings
        std::unordered_map<std::string, std::string> settings;
        settings["P2PPort"] = "10333";
        settings["RPCPort"] = "10332";
        settings["MemoryPoolCapacity"] = "50000";

        // Create node
        node_ = std::make_shared<Node>(storeProvider_, settings);
    }

    std::shared_ptr<StoreProvider> storeProvider_;
    std::shared_ptr<Node> node_;
};

TEST_F(NodeTest, Constructor)
{
    EXPECT_EQ(node_->GetStoreProvider(), storeProvider_);
    EXPECT_EQ(node_->GetSettings().size(), 3);
    EXPECT_EQ(node_->GetSettings().at("P2PPort"), "10333");
    EXPECT_EQ(node_->GetSettings().at("RPCPort"), "10332");
    EXPECT_EQ(node_->GetSettings().at("MemoryPoolCapacity"), "50000");
    EXPECT_NE(node_->GetBlockchain(), nullptr);
    EXPECT_NE(node_->GetMemoryPool(), nullptr);
    EXPECT_NE(node_->GetP2PServer(), nullptr);
    EXPECT_FALSE(node_->IsRunning());
}

TEST_F(NodeTest, StartStop)
{
    // Start node
    node_->Start();
    EXPECT_TRUE(node_->IsRunning());

    // Stop node
    node_->Stop();
    EXPECT_FALSE(node_->IsRunning());
}

TEST_F(NodeTest, GetBlockHeight)
{
    EXPECT_EQ(node_->GetBlockHeight(), 0);
}

TEST_F(NodeTest, GetBlock)
{
    // Get non-existent block
    auto block1 = node_->GetBlock(UInt256());
    EXPECT_EQ(block1, nullptr);

    auto block2 = node_->GetBlock(1);
    EXPECT_EQ(block2, nullptr);
}

TEST_F(NodeTest, GetBlockHeader)
{
    // Get non-existent block header
    auto header1 = node_->GetBlockHeader(UInt256());
    EXPECT_EQ(header1, nullptr);

    auto header2 = node_->GetBlockHeader(1);
    EXPECT_EQ(header2, nullptr);
}

TEST_F(NodeTest, GetTransaction)
{
    // Get non-existent transaction
    auto tx = node_->GetTransaction(UInt256());
    EXPECT_EQ(tx, nullptr);
}

TEST_F(NodeTest, GetContract)
{
    // Get non-existent contract
    auto contract = node_->GetContract(UInt160());
    EXPECT_EQ(contract, nullptr);
}

TEST_F(NodeTest, GetStorageValue)
{
    // Get non-existent storage value
    auto value = node_->GetStorageValue(UInt160(), ByteVector());
    EXPECT_TRUE(value.IsEmpty());
}

TEST_F(NodeTest, RelayTransaction)
{
    // Create transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetVersion(0);

    // Relay transaction
    bool result = node_->RelayTransaction(tx);
    EXPECT_FALSE(result);  // Should fail because transaction is invalid
}

TEST_F(NodeTest, RelayBlock)
{
    // Create block
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPrevHash(UInt256());
    block->SetMerkleRoot(UInt256());
    block->SetTimestamp(0);
    block->SetIndex(1);
    block->SetNextConsensus(UInt160());

    // Relay block
    bool result = node_->RelayBlock(block);
    EXPECT_TRUE(result);
}

TEST_F(NodeTest, Callbacks)
{
    // Register block persistence callback
    bool blockCallbackCalled = false;
    auto blockCallback = [&](std::shared_ptr<Block> block) { blockCallbackCalled = true; };
    int32_t blockCallbackId = node_->RegisterBlockPersistenceCallback(blockCallback);

    // Register transaction execution callback
    bool txCallbackCalled = false;
    auto txCallback = [&](std::shared_ptr<Transaction> tx) { txCallbackCalled = true; };
    int32_t txCallbackId = node_->RegisterTransactionExecutionCallback(txCallback);

    // Create block
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPrevHash(UInt256());
    block->SetMerkleRoot(UInt256());
    block->SetTimestamp(0);
    block->SetIndex(1);
    block->SetNextConsensus(UInt160());

    // Create transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetVersion(0);

    // Call callbacks
    node_->OnNewBlock(block);
    node_->OnNewTransaction(tx);

    // Check if callbacks were called
    EXPECT_TRUE(blockCallbackCalled);
    EXPECT_TRUE(txCallbackCalled);

    // Unregister callbacks
    node_->UnregisterBlockPersistenceCallback(blockCallbackId);
    node_->UnregisterTransactionExecutionCallback(txCallbackId);

    // Reset flags
    blockCallbackCalled = false;
    txCallbackCalled = false;

    // Call callbacks again
    node_->OnNewBlock(block);
    node_->OnNewTransaction(tx);

    // Check if callbacks were called
    EXPECT_FALSE(blockCallbackCalled);
    EXPECT_FALSE(txCallbackCalled);
}

TEST_F(NodeTest, ProcessBlock)
{
    // Create block
    auto block = std::make_shared<Block>();
    block->SetVersion(0);
    block->SetPrevHash(UInt256());
    block->SetMerkleRoot(UInt256());
    block->SetTimestamp(0);
    block->SetIndex(1);
    block->SetNextConsensus(UInt160());

    // Process block
    bool result = node_->ProcessBlock(block);
    EXPECT_FALSE(result);  // Should fail because block is invalid
}

TEST_F(NodeTest, ProcessTransaction)
{
    // Create transaction
    auto tx = std::make_shared<Transaction>();
    tx->SetVersion(0);

    // Process transaction
    bool result = node_->ProcessTransaction(tx);
    EXPECT_FALSE(result);  // Should fail because transaction is invalid
}

TEST(RPCRequestTest, Constructor)
{
    RPCRequest request;
    EXPECT_EQ(request.GetJsonRPC(), "2.0");
    EXPECT_TRUE(request.GetMethod().empty());
    EXPECT_TRUE(request.GetParams().empty());
    EXPECT_TRUE(request.GetId().empty());
}

TEST(RPCRequestTest, SettersAndGetters)
{
    RPCRequest request;

    // JsonRPC
    request.SetJsonRPC("2.0");
    EXPECT_EQ(request.GetJsonRPC(), "2.0");

    // Method
    request.SetMethod("getblockcount");
    EXPECT_EQ(request.GetMethod(), "getblockcount");

    // Params
    std::vector<std::string> params = {"param1", "param2"};
    request.SetParams(params);
    EXPECT_EQ(request.GetParams().size(), 2);
    EXPECT_EQ(request.GetParams()[0], "param1");
    EXPECT_EQ(request.GetParams()[1], "param2");

    // Id
    request.SetId("1");
    EXPECT_EQ(request.GetId(), "1");
}

TEST(RPCRequestTest, Serialization)
{
    RPCRequest request;
    request.SetJsonRPC("2.0");
    request.SetMethod("getblockcount");
    request.SetParams({"param1", "param2"});
    request.SetId("1");

    // Serialize
    nlohmann::json json = request.ToJson();

    // Check
    EXPECT_EQ(json["jsonrpc"], "2.0");
    EXPECT_EQ(json["method"], "getblockcount");
    EXPECT_EQ(json["params"].size(), 2);
    EXPECT_EQ(json["params"][0], "param1");
    EXPECT_EQ(json["params"][1], "param2");
    EXPECT_EQ(json["id"], "1");

    // Deserialize
    RPCRequest request2;
    request2.FromJson(json);

    // Check
    EXPECT_EQ(request2.GetJsonRPC(), "2.0");
    EXPECT_EQ(request2.GetMethod(), "getblockcount");
    EXPECT_EQ(request2.GetParams().size(), 2);
    EXPECT_EQ(request2.GetParams()[0], "param1");
    EXPECT_EQ(request2.GetParams()[1], "param2");
    EXPECT_EQ(request2.GetId(), "1");
}

TEST(RPCResponseTest, Constructor)
{
    RPCResponse response;
    EXPECT_EQ(response.GetJsonRPC(), "2.0");
    EXPECT_TRUE(response.GetResult().is_null());
    EXPECT_TRUE(response.GetError().is_null());
    EXPECT_TRUE(response.GetId().empty());
}

TEST(RPCResponseTest, SettersAndGetters)
{
    RPCResponse response;

    // JsonRPC
    response.SetJsonRPC("2.0");
    EXPECT_EQ(response.GetJsonRPC(), "2.0");

    // Result
    nlohmann::json result = 123;
    response.SetResult(result);
    EXPECT_EQ(response.GetResult(), 123);

    // Error
    nlohmann::json error;
    error["code"] = -32700;
    error["message"] = "Parse error";
    response.SetError(error);
    EXPECT_EQ(response.GetError()["code"], -32700);
    EXPECT_EQ(response.GetError()["message"], "Parse error");

    // Id
    response.SetId("1");
    EXPECT_EQ(response.GetId(), "1");
}

TEST(RPCResponseTest, Serialization)
{
    RPCResponse response;
    response.SetJsonRPC("2.0");
    response.SetResult(123);
    response.SetId("1");

    // Serialize
    nlohmann::json json = response.ToJson();

    // Check
    EXPECT_EQ(json["jsonrpc"], "2.0");
    EXPECT_EQ(json["result"], 123);
    EXPECT_FALSE(json.contains("error"));
    EXPECT_EQ(json["id"], "1");

    // Deserialize
    RPCResponse response2;
    response2.FromJson(json);

    // Check
    EXPECT_EQ(response2.GetJsonRPC(), "2.0");
    EXPECT_EQ(response2.GetResult(), 123);
    EXPECT_TRUE(response2.GetError().is_null());
    EXPECT_EQ(response2.GetId(), "1");

    // Response with error
    RPCResponse response3;
    response3.SetJsonRPC("2.0");
    nlohmann::json error;
    error["code"] = -32700;
    error["message"] = "Parse error";
    response3.SetError(error);
    response3.SetId("1");

    // Serialize
    json = response3.ToJson();

    // Check
    EXPECT_EQ(json["jsonrpc"], "2.0");
    EXPECT_FALSE(json.contains("result"));
    EXPECT_EQ(json["error"]["code"], -32700);
    EXPECT_EQ(json["error"]["message"], "Parse error");
    EXPECT_EQ(json["id"], "1");

    // Deserialize
    RPCResponse response4;
    response4.FromJson(json);

    // Check
    EXPECT_EQ(response4.GetJsonRPC(), "2.0");
    EXPECT_TRUE(response4.GetResult().is_null());
    EXPECT_EQ(response4.GetError()["code"], -32700);
    EXPECT_EQ(response4.GetError()["message"], "Parse error");
    EXPECT_EQ(response4.GetId(), "1");
}
