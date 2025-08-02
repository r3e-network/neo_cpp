#include <gtest/gtest.h>
#include <neo/rpc/rpc_response.h>

namespace neo::rpc::tests
{
class RpcResponseTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Test setup
    }
};

TEST_F(RpcResponseTest, TestDefaultConstructor)
{
    RpcResponse response;

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_TRUE(response.GetResult().is_null());
    EXPECT_TRUE(response.GetError().is_null());
    EXPECT_TRUE(response.GetId().is_null());
}

TEST_F(RpcResponseTest, TestSetters)
{
    RpcResponse response;

    response.SetJsonRpc("2.0");

    nlohmann::json result = {{"block_count", 12345}};
    response.SetResult(result);

    nlohmann::json id = "test_id";
    response.SetId(id);

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_EQ(result, response.GetResult());
    EXPECT_EQ(id, response.GetId());
}

TEST_F(RpcResponseTest, TestErrorResponse)
{
    RpcResponse response;

    nlohmann::json error = {
        {"code", -32601},
        {"message", "Method not found"},
        {"data", "Additional error data"}
    };

    response.SetError(error);
    response.SetId(1);

    EXPECT_EQ(-32601, response.GetError()["code"]);
    EXPECT_EQ("Method not found", response.GetError()["message"]);
    EXPECT_EQ("Additional error data", response.GetError()["data"]);
    EXPECT_EQ(1, response.GetId());
}

TEST_F(RpcResponseTest, TestToJson)
{
    RpcResponse response;

    nlohmann::json result = {{"version", "3.0.0"}, {"tcpport", 10333}, {"wsport", 10334}};

    response.SetResult(result);
    response.SetId(42);

    nlohmann::json json = response.ToJson();

    EXPECT_EQ("2.0", json["jsonrpc"]);
    EXPECT_EQ(result, json["result"]);
    EXPECT_EQ(42, json["id"]);
    EXPECT_TRUE(!json.contains("error") || json["error"].is_null());
}

TEST_F(RpcResponseTest, TestToJsonWithError)
{
    RpcResponse response;

    nlohmann::json error = {
        {"code", -32602},
        {"message", "Invalid params"}
    };

    response.SetError(error);
    response.SetId("error_test");

    nlohmann::json json = response.ToJson();

    EXPECT_EQ("2.0", json["jsonrpc"]);
    EXPECT_TRUE(!json.contains("result") || json["result"].is_null());
    EXPECT_EQ(-32602, json["error"]["code"]);
    EXPECT_EQ("Invalid params", json["error"]["message"]);
    EXPECT_EQ("error_test", json["id"]);
}

TEST_F(RpcResponseTest, TestFromJson)
{
    nlohmann::json json = {{"jsonrpc", "2.0"}, {"result", {{"block_count", 54321}}}, {"id", 123}};

    RpcResponse response = RpcResponse::FromJson(json);

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_EQ(54321, response.GetResult()["block_count"]);
    EXPECT_EQ(123, response.GetId());
    EXPECT_TRUE(response.GetError().is_null());
}

TEST_F(RpcResponseTest, TestFromJsonWithError)
{
    nlohmann::json json = {{"jsonrpc", "2.0"},
                           {"error", {{"code", -32700}, {"message", "Parse error"}, {"data", "Invalid JSON"}}},
                           {"id", nullptr}};

    RpcResponse response = RpcResponse::FromJson(json);

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_TRUE(response.GetResult().is_null());
    EXPECT_EQ(-32700, response.GetError()["code"]);
    EXPECT_EQ("Parse error", response.GetError()["message"]);
    EXPECT_EQ("Invalid JSON", response.GetError()["data"]);
    EXPECT_TRUE(response.GetId().is_null());
}

TEST_F(RpcResponseTest, TestRoundTrip)
{
    // Test successful response round trip
    RpcResponse original;
    nlohmann::json result = {{"hash", "0x1234567890abcdef"}, {"size", 1024}, {"confirmations", 6}};
    original.SetResult(result);
    original.SetId("round_trip_test");

    nlohmann::json json = original.ToJson();
    RpcResponse deserialized = RpcResponse::FromJson(json);

    EXPECT_EQ(original.GetJsonRpc(), deserialized.GetJsonRpc());
    EXPECT_EQ(original.GetResult(), deserialized.GetResult());
    EXPECT_EQ(original.GetId(), deserialized.GetId());
    EXPECT_EQ(original.GetError(), deserialized.GetError());
}

TEST_F(RpcResponseTest, TestErrorRoundTrip)
{
    // Test error response round trip
    RpcResponse original;

    nlohmann::json error = {
        {"code", -32603},
        {"message", "Internal error"},
        {"data", "Server encountered an internal error"}
    };

    original.SetError(error);
    original.SetId(999);

    nlohmann::json json = original.ToJson();
    RpcResponse deserialized = RpcResponse::FromJson(json);

    EXPECT_EQ(original.GetJsonRpc(), deserialized.GetJsonRpc());
    EXPECT_TRUE(deserialized.GetResult().is_null());
    EXPECT_EQ(original.GetError()["code"], deserialized.GetError()["code"]);
    EXPECT_EQ(original.GetError()["message"], deserialized.GetError()["message"]);
    EXPECT_EQ(original.GetError()["data"], deserialized.GetError()["data"]);
    EXPECT_EQ(original.GetId(), deserialized.GetId());
}

TEST_F(RpcResponseTest, TestCommonSuccessResponses)
{
    // Test common Neo RPC success responses

    // getversion response
    RpcResponse version_response;
    version_response.SetResult(
        {{"tcpport", 10333}, {"wsport", 10334}, {"nonce", 1234567890}, {"useragent", "/Neo:3.0.0/"}});
    version_response.SetId(1);

    auto json1 = version_response.ToJson();
    auto deserialized1 = RpcResponse::FromJson(json1);
    EXPECT_EQ(10333, deserialized1.GetResult()["tcpport"]);

    // getblockcount response
    RpcResponse blockcount_response;
    blockcount_response.SetResult(12345);
    blockcount_response.SetId(2);

    auto json2 = blockcount_response.ToJson();
    auto deserialized2 = RpcResponse::FromJson(json2);
    EXPECT_EQ(12345, deserialized2.GetResult());

    // getbestblockhash response
    RpcResponse hash_response;
    hash_response.SetResult("0x1234567890abcdef1234567890abcdef12345678");
    hash_response.SetId(3);

    auto json3 = hash_response.ToJson();
    auto deserialized3 = RpcResponse::FromJson(json3);
    EXPECT_EQ("0x1234567890abcdef1234567890abcdef12345678", deserialized3.GetResult());
}

TEST_F(RpcResponseTest, TestCommonErrorResponses)
{
    // Test common JSON-RPC error codes

    struct ErrorTest
    {
        int code;
        std::string message;
    };

    std::vector<ErrorTest> error_tests = {{-32700, "Parse error"},      {-32600, "Invalid Request"},
                                          {-32601, "Method not found"}, {-32602, "Invalid params"},
                                          {-32603, "Internal error"},   {-32000, "Server error"}};

    for (size_t i = 0; i < error_tests.size(); ++i)
    {
        RpcResponse response;

        nlohmann::json error = {
            {"code", error_tests[i].code},
            {"message", error_tests[i].message}
        };

        response.SetError(error);
        response.SetId(static_cast<int>(i));

        auto json = response.ToJson();
        auto deserialized = RpcResponse::FromJson(json);

        EXPECT_EQ(error_tests[i].code, deserialized.GetError()["code"].get<int>());
        EXPECT_EQ(error_tests[i].message, deserialized.GetError()["message"].get<std::string>());
        EXPECT_EQ(static_cast<int>(i), deserialized.GetId());
    }
}

TEST_F(RpcResponseTest, TestComplexResultTypes)
{
    // Test with complex nested result
    nlohmann::json complex_result = {
        {"block",
         {{"hash", "0x1234567890abcdef"},
          {"size", 1024},
          {"version", 0},
          {"previousblockhash", "0xabcdef1234567890"},
          {"merkleroot", "0x9876543210fedcba"},
          {"time", 1234567890},
          {"index", 12345},
          {"nonce", "0x1234567890abcdef"},
          {"nextconsensus", "NZNos2WqwVfNUXNj5VEqvvPzAqze3RXyP3"},
          {"witnesses", nlohmann::json::array({{{"invocation", "0x123456"}, {"verification", "0x789abc"}}})},
          {"tx", nlohmann::json::array({{{"hash", "0xfedcba0987654321"},
                                         {"size", 256},
                                         {"version", 0},
                                         {"nonce", 123456789},
                                         {"sender", "NZNos2WqwVfNUXNj5VEqvvPzAqze3RXyP3"},
                                         {"sysfee", "1000000"},
                                         {"netfee", "100000"},
                                         {"validuntilblock", 12350},
                                         {"script", "0x0c14abcdef1234567890"}}})}}}};

    RpcResponse response;
    response.SetResult(complex_result);
    response.SetId("complex_test");

    auto json = response.ToJson();
    auto deserialized = RpcResponse::FromJson(json);

    EXPECT_EQ(complex_result, deserialized.GetResult());
    EXPECT_EQ("complex_test", deserialized.GetId());
}

TEST_F(RpcResponseTest, TestPartialJson)
{
    // Test with minimal JSON
    nlohmann::json minimal_json = {{"jsonrpc", "2.0"}, {"id", 1}};

    RpcResponse response = RpcResponse::FromJson(minimal_json);

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_TRUE(response.GetResult().is_null());
    EXPECT_TRUE(response.GetError().is_null());
    EXPECT_EQ(1, response.GetId());
}

TEST_F(RpcResponseTest, TestLargeResponse)
{
    // Test with large result data
    nlohmann::json large_result = nlohmann::json::array();
    for (int i = 0; i < 1000; ++i)
    {
        large_result.push_back({{"id", i}, {"data", "large_data_item_" + std::to_string(i)}});
    }

    RpcResponse response;
    response.SetResult(large_result);
    response.SetId("large_test");

    auto json = response.ToJson();
    auto deserialized = RpcResponse::FromJson(json);

    EXPECT_EQ(1000, deserialized.GetResult().size());
    EXPECT_EQ("large_test", deserialized.GetId());
    EXPECT_EQ(999, deserialized.GetResult()[999]["id"]);
}

TEST_F(RpcResponseTest, TestCreateSuccessResponse)
{
    // Test CreateSuccessResponse static method
    nlohmann::json result = {{"status", "success"}, {"value", 42}};
    RpcResponse response = RpcResponse::CreateSuccessResponse(123, result);

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_EQ(result, response.GetResult());
    EXPECT_TRUE(response.GetError().is_null());
    EXPECT_EQ(123, response.GetId());
}

TEST_F(RpcResponseTest, TestCreateErrorResponse)
{
    // Test CreateErrorResponse static method
    RpcResponse response = RpcResponse::CreateErrorResponse("test_id", -32600, "Invalid Request", "Missing required field");

    EXPECT_EQ("2.0", response.GetJsonRpc());
    EXPECT_TRUE(response.GetResult().is_null());
    EXPECT_EQ(-32600, response.GetError()["code"]);
    EXPECT_EQ("Invalid Request", response.GetError()["message"]);
    EXPECT_EQ("Missing required field", response.GetError()["data"]);
    EXPECT_EQ("test_id", response.GetId());
}
}  // namespace neo::rpc::tests