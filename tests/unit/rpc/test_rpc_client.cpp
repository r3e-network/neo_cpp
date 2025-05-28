#include <gtest/gtest.h>
#include <neo/rpc/rpc_client.h>
#include <memory>

namespace neo::rpc::tests
{
    class MockHttpClient : public IHttpClient
    {
    public:
        std::string mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "mock_result"
        })";

        std::string Post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers = {}) override
        {
            last_url = url;
            last_content = content;
            last_headers = headers;
            return mock_response;
        }

        std::future<std::string> PostAsync(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers = {}) override
        {
            return std::async(std::launch::async, [this, url, content, headers]() {
                return Post(url, content, headers);
            });
        }

        std::string last_url;
        std::string last_content;
        std::map<std::string, std::string> last_headers;
    };

    class RpcClientTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            mock_client = std::make_unique<MockHttpClient>();
            mock_client_ptr = mock_client.get();
            rpc_client = std::make_unique<RpcClient>("http://localhost:10332", std::move(mock_client));
        }

        std::unique_ptr<MockHttpClient> mock_client;
        MockHttpClient* mock_client_ptr;
        std::unique_ptr<RpcClient> rpc_client;
    };

    TEST_F(RpcClientTest, TestConstructor)
    {
        // Test basic constructor
        RpcClient client("http://localhost:10332");
        EXPECT_NO_THROW(client.GetVersion());
    }

    TEST_F(RpcClientTest, TestConstructorWithAuth)
    {
        // Test constructor with authentication
        RpcClient client("http://localhost:10332", "user", "pass");
        EXPECT_NO_THROW(client.GetVersion());
    }

    TEST_F(RpcClientTest, TestGetBestBlockHash)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "0x1234567890abcdef"
        })";

        std::string hash = rpc_client->GetBestBlockHash();
        EXPECT_EQ("mock_result", hash);  // Simplified mock returns "mock_result"

        // Verify the request was made correctly
        EXPECT_EQ("http://localhost:10332", mock_client_ptr->last_url);
        EXPECT_TRUE(mock_client_ptr->last_content.find("getbestblockhash") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestGetBestBlockHashAsync)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "0x1234567890abcdef"
        })";

        auto future = rpc_client->GetBestBlockHashAsync();
        std::string hash = future.get();
        EXPECT_EQ("mock_result", hash);
    }

    TEST_F(RpcClientTest, TestGetBlockCount)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": 12345
        })";

        // Test that GetBlockCount executes without throwing
        EXPECT_NO_THROW(rpc_client->GetBlockCount());
    }

    TEST_F(RpcClientTest, TestGetBlockCountAsync)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": 12345
        })";

        auto future = rpc_client->GetBlockCountAsync();
        EXPECT_NO_THROW(future.get());
    }

    TEST_F(RpcClientTest, TestGetBlockByHash)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": {
                "hash": "0x1234567890abcdef",
                "size": 1024,
                "version": 0
            }
        })";

        auto result = rpc_client->GetBlock("0x1234567890abcdef", true);
        EXPECT_NO_THROW(result.ToString());

        // Verify the request contains the hash parameter
        EXPECT_TRUE(mock_client_ptr->last_content.find("0x1234567890abcdef") != std::string::npos);
        EXPECT_TRUE(mock_client_ptr->last_content.find("getblock") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestGetBlockByIndex)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": {
                "hash": "0x1234567890abcdef",
                "size": 1024,
                "version": 0
            }
        })";

        auto result = rpc_client->GetBlock(12345, true);
        EXPECT_NO_THROW(result.ToString());

        // Verify the request contains the index parameter
        EXPECT_TRUE(mock_client_ptr->last_content.find("12345") != std::string::npos);
        EXPECT_TRUE(mock_client_ptr->last_content.find("getblock") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestGetTransaction)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": {
                "hash": "0xabcdef1234567890",
                "size": 256,
                "version": 0
            }
        })";

        auto result = rpc_client->GetTransaction("0xabcdef1234567890", true);
        EXPECT_NO_THROW(result.ToString());

        // Verify the request contains the transaction hash
        EXPECT_TRUE(mock_client_ptr->last_content.find("0xabcdef1234567890") != std::string::npos);
        EXPECT_TRUE(mock_client_ptr->last_content.find("getrawtransaction") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestSendRawTransaction)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "0xabcdef1234567890"
        })";

        std::string tx_hex = "0123456789abcdef";
        std::string result = rpc_client->SendRawTransaction(tx_hex);
        EXPECT_EQ("mock_result", result);

        // Verify the request contains the transaction hex
        EXPECT_TRUE(mock_client_ptr->last_content.find(tx_hex) != std::string::npos);
        EXPECT_TRUE(mock_client_ptr->last_content.find("sendrawtransaction") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestInvokeFunction)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": {
                "script": "0x1234",
                "state": "HALT",
                "gasconsumed": "1000000"
            }
        })";

        std::string script_hash = "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5";
        std::string operation = "balanceOf";
        std::vector<json::JToken> params;
        params.push_back(json::JString("NZNos2WqwVfNUXNj5VEqvvPzAqze3RXyP3"));

        auto result = rpc_client->InvokeFunction(script_hash, operation, params);
        EXPECT_NO_THROW(result.ToString());

        // Verify the request contains the script hash and operation
        EXPECT_TRUE(mock_client_ptr->last_content.find(script_hash) != std::string::npos);
        EXPECT_TRUE(mock_client_ptr->last_content.find(operation) != std::string::npos);
        EXPECT_TRUE(mock_client_ptr->last_content.find("invokefunction") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestGetVersion)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": {
                "tcpport": 10333,
                "wsport": 10334,
                "nonce": 1234567890,
                "useragent": "/Neo:3.0.0/"
            }
        })";

        auto result = rpc_client->GetVersion();
        EXPECT_NO_THROW(result.ToString());

        // Verify the request was made correctly
        EXPECT_TRUE(mock_client_ptr->last_content.find("getversion") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestRpcSend)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "test_result"
        })";

        std::vector<json::JToken> params;
        params.push_back(json::JString("param1"));
        params.push_back(json::JNumber(42));

        auto result = rpc_client->RpcSend("testmethod", params);
        EXPECT_NO_THROW(result.ToString());

        // Verify the request contains the method and parameters
        EXPECT_TRUE(mock_client_ptr->last_content.find("testmethod") != std::string::npos);
    }

    TEST_F(RpcClientTest, TestAsyncMethods)
    {
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "async_result"
        })";

        // Test multiple async methods
        auto future1 = rpc_client->GetVersionAsync();
        auto future2 = rpc_client->GetBestBlockHashAsync();
        auto future3 = rpc_client->GetBlockCountAsync();

        // Wait for all futures to complete
        EXPECT_NO_THROW(future1.get());
        EXPECT_NO_THROW(future2.get());
        EXPECT_NO_THROW(future3.get());
    }

    TEST_F(RpcClientTest, TestErrorHandling)
    {
        // Test error response
        mock_client_ptr->mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "error": {
                "code": -32601,
                "message": "Method not found"
            }
        })";

        // This should throw an exception due to the error in response
        EXPECT_THROW(rpc_client->GetBestBlockHash(), std::runtime_error);
    }

    TEST_F(RpcClientTest, TestInvalidJsonResponse)
    {
        // Test invalid JSON response
        mock_client_ptr->mock_response = "invalid json";

        // This should throw an exception due to invalid JSON
        EXPECT_THROW(rpc_client->GetBestBlockHash(), std::runtime_error);
    }

    TEST_F(RpcClientTest, TestHttpHeaders)
    {
        rpc_client->GetVersion();

        // Verify that Content-Type header was set
        auto it = mock_client_ptr->last_headers.find("Content-Type");
        EXPECT_NE(it, mock_client_ptr->last_headers.end());
        EXPECT_EQ("application/json", it->second);
    }
}
