#include <gtest/gtest.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/oracle_request.h>
#include <neo/smartcontract/native/id_list.h>
#include <neo/smartcontract/native/gas_token.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/persistence/memory_store_view.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/ledger/transaction.h>
#include <neo/ledger/oracle_response.h>
#include <sstream>

using namespace neo::smartcontract::native;
using namespace neo::persistence;
using namespace neo::io;
using namespace neo::vm;
using namespace neo::ledger;
using namespace neo::cryptography;
using namespace neo::cryptography::ecc;

class OracleContractTest : public ::testing::Test
{
protected:
    std::shared_ptr<MemoryStoreView> snapshot;
    std::shared_ptr<OracleContract> oracleContract;
    std::shared_ptr<GasToken> gasToken;
    std::shared_ptr<RoleManagement> roleManagement;
    std::shared_ptr<Block> block;
    
    void SetUp() override
    {
        snapshot = std::make_shared<MemoryStoreView>();
        oracleContract = OracleContract::GetInstance();
        gasToken = GasToken::GetInstance();
        roleManagement = RoleManagement::GetInstance();
        
        // Initialize contracts
        oracleContract->Initialize();
        gasToken->Initialize();
        roleManagement->Initialize();
        
        // Create a block
        block = std::make_shared<Block>();
        auto header = std::make_shared<Header>();
        header->SetIndex(0);
        block->SetHeader(header);
    }
};

TEST_F(OracleContractTest, TestGetPrice)
{
    // Default price should be 1000000
    ASSERT_EQ(oracleContract->GetPrice(snapshot), 1000000);
    
    // Set a new price
    oracleContract->SetPrice(snapshot, 2000000);
    
    // Check if the price was updated
    ASSERT_EQ(oracleContract->GetPrice(snapshot), 2000000);
}

TEST_F(OracleContractTest, TestOracleRequest)
{
    // Create an OracleRequest
    io::UInt256 originalTxid;
    std::memset(originalTxid.Data(), 1, originalTxid.Size());
    
    int64_t gasForResponse = 1000000;
    std::string url = "https://example.com/api";
    std::string filter = "$.data";
    
    io::UInt160 callbackContract;
    std::memset(callbackContract.Data(), 2, callbackContract.Size());
    
    std::string callbackMethod = "callback";
    io::ByteVector userData(io::ByteSpan(reinterpret_cast<const uint8_t*>("test"), 4));
    
    OracleRequest request(originalTxid, gasForResponse, url, filter, callbackContract, callbackMethod, userData);
    
    // Test getters
    ASSERT_EQ(request.GetOriginalTxid(), originalTxid);
    ASSERT_EQ(request.GetGasForResponse(), gasForResponse);
    ASSERT_EQ(request.GetUrl(), url);
    ASSERT_EQ(request.GetFilter(), filter);
    ASSERT_EQ(request.GetCallbackContract(), callbackContract);
    ASSERT_EQ(request.GetCallbackMethod(), callbackMethod);
    ASSERT_EQ(request.GetUserData(), userData);
    
    // Test serialization
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    request.Serialize(writer);
    std::string data = stream.str();
    
    // Test deserialization
    std::istringstream inStream(data);
    io::BinaryReader reader(inStream);
    OracleRequest deserializedRequest;
    deserializedRequest.Deserialize(reader);
    
    // Check if deserialized request matches the original
    ASSERT_EQ(deserializedRequest.GetOriginalTxid(), originalTxid);
    ASSERT_EQ(deserializedRequest.GetGasForResponse(), gasForResponse);
    ASSERT_EQ(deserializedRequest.GetUrl(), url);
    ASSERT_EQ(deserializedRequest.GetFilter(), filter);
    ASSERT_EQ(deserializedRequest.GetCallbackContract(), callbackContract);
    ASSERT_EQ(deserializedRequest.GetCallbackMethod(), callbackMethod);
    ASSERT_EQ(deserializedRequest.GetUserData(), userData);
}

TEST_F(OracleContractTest, TestIdList)
{
    // Create an IdList
    IdList idList;
    
    // Test empty list
    ASSERT_EQ(idList.GetCount(), 0);
    
    // Add IDs
    idList.Add(1);
    idList.Add(2);
    idList.Add(3);
    
    // Test count
    ASSERT_EQ(idList.GetCount(), 3);
    
    // Test contains
    ASSERT_TRUE(idList.Contains(1));
    ASSERT_TRUE(idList.Contains(2));
    ASSERT_TRUE(idList.Contains(3));
    ASSERT_FALSE(idList.Contains(4));
    
    // Test remove
    ASSERT_TRUE(idList.Remove(2));
    ASSERT_EQ(idList.GetCount(), 2);
    ASSERT_FALSE(idList.Contains(2));
    
    // Test serialization
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    idList.Serialize(writer);
    std::string data = stream.str();
    
    // Test deserialization
    std::istringstream inStream(data);
    io::BinaryReader reader(inStream);
    IdList deserializedIdList;
    deserializedIdList.Deserialize(reader);
    
    // Check if deserialized list matches the original
    ASSERT_EQ(deserializedIdList.GetCount(), 2);
    ASSERT_TRUE(deserializedIdList.Contains(1));
    ASSERT_TRUE(deserializedIdList.Contains(3));
    ASSERT_FALSE(deserializedIdList.Contains(2));
}

TEST_F(OracleContractTest, TestCreateRequest)
{
    // Create application engine
    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot, 0, false);
    engine.SetPersistingBlock(block);
    
    // Create a request
    io::UInt256 originalTxid;
    std::memset(originalTxid.Data(), 1, originalTxid.Size());
    
    int64_t gasForResponse = 1000000;
    std::string url = "https://example.com/api";
    std::string filter = "$.data";
    
    io::UInt160 callbackContract;
    std::memset(callbackContract.Data(), 2, callbackContract.Size());
    
    std::string callbackMethod = "callback";
    io::ByteVector userData(io::ByteSpan(reinterpret_cast<const uint8_t*>("test"), 4));
    
    // Create the request
    uint64_t id = oracleContract->CreateRequest(snapshot, url, filter, callbackContract, callbackMethod, gasForResponse, userData, originalTxid);
    
    // Check if the request was created
    auto request = oracleContract->GetRequest(snapshot, id);
    
    // Check if the request matches the input
    ASSERT_EQ(request.GetOriginalTxid(), originalTxid);
    ASSERT_EQ(request.GetGasForResponse(), gasForResponse);
    ASSERT_EQ(request.GetUrl(), url);
    ASSERT_EQ(request.GetFilter(), filter);
    ASSERT_EQ(request.GetCallbackContract(), callbackContract);
    ASSERT_EQ(request.GetCallbackMethod(), callbackMethod);
    ASSERT_EQ(request.GetUserData(), userData);
    
    // Check if the request is in the ID list
    auto urlHash = oracleContract->GetUrlHash(url);
    auto idList = oracleContract->GetIdList(snapshot, urlHash);
    ASSERT_TRUE(idList.Contains(id));
}

TEST_F(OracleContractTest, TestGetRequests)
{
    // Create multiple requests
    io::UInt256 originalTxid;
    std::memset(originalTxid.Data(), 1, originalTxid.Size());
    
    int64_t gasForResponse = 1000000;
    std::string url = "https://example.com/api";
    std::string filter = "$.data";
    
    io::UInt160 callbackContract;
    std::memset(callbackContract.Data(), 2, callbackContract.Size());
    
    std::string callbackMethod = "callback";
    io::ByteVector userData(io::ByteSpan(reinterpret_cast<const uint8_t*>("test"), 4));
    
    // Create the requests
    uint64_t id1 = oracleContract->CreateRequest(snapshot, url, filter, callbackContract, callbackMethod, gasForResponse, userData, originalTxid);
    uint64_t id2 = oracleContract->CreateRequest(snapshot, url, filter, callbackContract, callbackMethod, gasForResponse, userData, originalTxid);
    
    // Get all requests
    auto requests = oracleContract->GetRequests(snapshot);
    
    // Check if both requests are returned
    ASSERT_EQ(requests.size(), 2);
    
    // Get requests by URL
    auto requestsByUrl = oracleContract->GetRequestsByUrl(snapshot, url);
    
    // Check if both requests are returned
    ASSERT_EQ(requestsByUrl.size(), 2);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
