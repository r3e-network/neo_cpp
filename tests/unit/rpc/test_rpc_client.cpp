#include <deque>
#include <gtest/gtest.h>
#include <memory>
#include <neo/rpc/rpc_client.h>

namespace neo::rpc::tests
{
namespace
{
std::string BuildResultResponse(const nlohmann::json& result)
{
    nlohmann::json response;
    response["jsonrpc"] = "2.0";
    response["id"] = 1;
    response["result"] = result;
    return response.dump();
}

std::string BuildErrorResponse(int code, const std::string& message)
{
    nlohmann::json response;
    response["jsonrpc"] = "2.0";
    response["id"] = 1;
    nlohmann::json error;
    error["code"] = code;
    error["message"] = message;
    response["error"] = error;
    return response.dump();
}
}  // namespace

class MockHttpClient : public IHttpClient
{
  public:
    std::string mock_response = BuildResultResponse("mock_result");

    std::string Post(const std::string& url, const std::string& content,
                     const std::map<std::string, std::string>& headers = {}) override
    {
        post_called = true;
        last_url = url;
        last_content = content;
        last_headers = headers;

        if (!queued_responses.empty())
        {
            auto front = queued_responses.front();
            queued_responses.pop_front();
            return front;
        }

        return mock_response;
    }

    std::future<std::string> PostAsync(const std::string& url, const std::string& content,
                                       const std::map<std::string, std::string>& headers = {}) override
    {
        post_async_called = true;
        return std::async(std::launch::async, [this, url, content, headers]() { return Post(url, content, headers); });
    }

    void QueueResponse(const std::string& response) { queued_responses.emplace_back(response); }

    bool post_called = false;
    bool post_async_called = false;
    std::string last_url;
    std::string last_content;
    std::map<std::string, std::string> last_headers;

  private:
    std::deque<std::string> queued_responses;
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

TEST_F(RpcClientTest, DefaultConstructorDoesNotThrow)
{
    EXPECT_NO_THROW({ RpcClient client("http://localhost:10332"); });
}

TEST_F(RpcClientTest, ConstructorWithAuthDoesNotThrow)
{
    EXPECT_NO_THROW({ RpcClient client("http://localhost:10332", "user", "pass"); });
}

TEST_F(RpcClientTest, GetBestBlockHashReturnsMockedResult)
{
    const std::string expected_hash = "0x1234567890abcdef";
    mock_client_ptr->mock_response = BuildResultResponse(expected_hash);

    const auto hash = rpc_client->GetBestBlockHash();

    EXPECT_EQ(expected_hash, hash);
    EXPECT_TRUE(mock_client_ptr->post_called);
    EXPECT_EQ("http://localhost:10332", mock_client_ptr->last_url);
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("getbestblockhash"));
}

TEST_F(RpcClientTest, GetBestBlockHashAsyncUsesAsyncTransport)
{
    const std::string expected_hash = "0xabcdef";
    mock_client_ptr->mock_response = BuildResultResponse(expected_hash);

    auto future = rpc_client->GetBestBlockHashAsync();

    EXPECT_EQ(expected_hash, future.get());
    EXPECT_TRUE(mock_client_ptr->post_async_called);
}

TEST_F(RpcClientTest, GetBlockCountParsesNumericResult)
{
    const uint32_t expected_count = 42;
    mock_client_ptr->mock_response = BuildResultResponse(expected_count);

    EXPECT_EQ(expected_count, rpc_client->GetBlockCount());
}

TEST_F(RpcClientTest, GetBlockByHashSerializesParameters)
{
    nlohmann::json block;
    block["hash"] = "0x123";
    block["size"] = 10;
    mock_client_ptr->mock_response = BuildResultResponse(block);

    auto result = rpc_client->GetBlock("0x123", true);
    EXPECT_EQ(block.dump(), result.dump());
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("0x123"));
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("getblock"));
}

TEST_F(RpcClientTest, GetBlockByIndexSerializesIndexParameter)
{
    nlohmann::json block;
    block["hash"] = "0x123";
    block["size"] = 10;
    mock_client_ptr->mock_response = BuildResultResponse(block);

    auto result = rpc_client->GetBlock(123u, true);
    EXPECT_EQ(block.dump(), result.dump());
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("123"));
}

TEST_F(RpcClientTest, GetTransactionUsesCorrectMethod)
{
    nlohmann::json tx;
    tx["hash"] = "0xabc";
    mock_client_ptr->mock_response = BuildResultResponse(tx);

    auto result = rpc_client->GetTransaction("0xabc", true);
    EXPECT_EQ(tx.dump(), result.dump());
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("getrawtransaction"));
}

TEST_F(RpcClientTest, SendRawTransactionReturnsResponseString)
{
    const std::string expected = "0xabc";
    mock_client_ptr->mock_response = BuildResultResponse(expected);

    const auto result = rpc_client->SendRawTransaction("012345");

    EXPECT_EQ(expected, result);
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("sendrawtransaction"));
}

TEST_F(RpcClientTest, InvokeFunctionIncludesScriptHashAndOperation)
{
    nlohmann::json invoke_result;
    invoke_result["state"] = "HALT";
    mock_client_ptr->mock_response = BuildResultResponse(invoke_result);

    const std::string script_hash = "0xef40";
    const std::string operation = "balanceOf";
    std::vector<nlohmann::json> params;
    params.emplace_back("address");

    auto result = rpc_client->InvokeFunction(script_hash, operation, params);
    EXPECT_EQ(invoke_result.dump(), result.dump());
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find(script_hash));
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find(operation));
}

TEST_F(RpcClientTest, RpcSendReturnsStructuredResult)
{
    nlohmann::json rpc_result;
    rpc_result["value"] = 5;
    mock_client_ptr->mock_response = BuildResultResponse(rpc_result);

    std::vector<nlohmann::json> params;
    params.emplace_back("param1");
    auto result = rpc_client->RpcSend("testmethod", params);

    EXPECT_EQ(rpc_result.dump(), result.dump());
    EXPECT_NE(std::string::npos, mock_client_ptr->last_content.find("testmethod"));
}

TEST_F(RpcClientTest, RpcSendAsyncReturnsResult)
{
    const std::string expected = "async";
    mock_client_ptr->mock_response = BuildResultResponse(expected);
    std::vector<nlohmann::json> params;
    params.emplace_back(1);

    auto future = rpc_client->RpcSendAsync("method", params);
    EXPECT_EQ(expected, future.get().get<std::string>());
}

TEST_F(RpcClientTest, AsyncMethodsConsumeQueuedResponses)
{
    mock_client_ptr->QueueResponse(BuildResultResponse(nlohmann::json{{"tcpport", 10333}}));
    mock_client_ptr->QueueResponse(BuildResultResponse("0xhash"));
    mock_client_ptr->QueueResponse(BuildResultResponse(123u));

    auto version_future = rpc_client->GetVersionAsync();
    auto hash_future = rpc_client->GetBestBlockHashAsync();
    auto count_future = rpc_client->GetBlockCountAsync();

    auto version = version_future.get();
    EXPECT_EQ(10333, version.at("tcpport").get<int>());
    EXPECT_EQ("0xhash", hash_future.get());
    EXPECT_EQ(123u, count_future.get());
}

TEST_F(RpcClientTest, ThrowsOnRpcErrorWhenThrowingEnabled)
{
    mock_client_ptr->mock_response = BuildErrorResponse(-32601, "Method not found");
    EXPECT_THROW(rpc_client->GetBestBlockHash(), std::runtime_error);
}

TEST_F(RpcClientTest, ReturnsErrorPayloadWhenThrowDisabled)
{
    mock_client_ptr->mock_response = BuildErrorResponse(-500, "InsufficientFunds");
    auto request = RpcRequest("2.0", "sendrawtransaction", nlohmann::json::array(), 1);
    auto response = rpc_client->Send(request, false);

    ASSERT_FALSE(response.GetError().is_null());
    EXPECT_EQ(-500, response.GetError().at("code").get<int>());
    EXPECT_EQ("InsufficientFunds", response.GetError().at("message").get<std::string>());
}

TEST_F(RpcClientTest, InvalidJsonResponseThrows)
{
    mock_client_ptr->mock_response = "not json";
    EXPECT_THROW(rpc_client->GetBestBlockHash(), std::runtime_error);
}

TEST_F(RpcClientTest, HttpHeadersIncludeContentType)
{
    mock_client_ptr->mock_response = BuildResultResponse(nlohmann::json{{"tcpport", 10333}});
    rpc_client->GetVersion();

    auto it = mock_client_ptr->last_headers.find("Content-Type");
    ASSERT_NE(mock_client_ptr->last_headers.end(), it);
    EXPECT_EQ("application/json", it->second);
}
}  // namespace neo::rpc::tests
