#include <gtest/gtest.h>
#include <neo/rpc/rpc_request.h>

namespace neo::rpc::tests
{
    class RpcRequestTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test setup
        }
    };

    TEST_F(RpcRequestTest, TestDefaultConstructor)
    {
        RpcRequest request;
        
        EXPECT_EQ("2.0", request.GetJsonRpc());
        EXPECT_TRUE(request.GetMethod().empty());
        EXPECT_TRUE(request.GetParams().is_null());
        EXPECT_TRUE(request.GetId().is_null());
    }

    TEST_F(RpcRequestTest, TestParameterizedConstructor)
    {
        nlohmann::json params = nlohmann::json::array({"param1", "param2"});
        nlohmann::json id = 123;
        
        RpcRequest request("2.0", "test_method", params, id);
        
        EXPECT_EQ("2.0", request.GetJsonRpc());
        EXPECT_EQ("test_method", request.GetMethod());
        EXPECT_EQ(params, request.GetParams());
        EXPECT_EQ(id, request.GetId());
    }

    TEST_F(RpcRequestTest, TestSetters)
    {
        RpcRequest request;
        
        request.SetJsonRpc("2.0");
        request.SetMethod("getblockcount");
        
        nlohmann::json params = nlohmann::json::array();
        request.SetParams(params);
        
        nlohmann::json id = "test_id";
        request.SetId(id);
        
        EXPECT_EQ("2.0", request.GetJsonRpc());
        EXPECT_EQ("getblockcount", request.GetMethod());
        EXPECT_EQ(params, request.GetParams());
        EXPECT_EQ(id, request.GetId());
    }

    TEST_F(RpcRequestTest, TestToJson)
    {
        nlohmann::json params = nlohmann::json::array({"param1", 42});
        nlohmann::json id = 1;
        
        RpcRequest request("2.0", "test_method", params, id);
        nlohmann::json json = request.ToJson();
        
        EXPECT_EQ("2.0", json["jsonrpc"]);
        EXPECT_EQ("test_method", json["method"]);
        EXPECT_EQ(params, json["params"]);
        EXPECT_EQ(id, json["id"]);
    }

    TEST_F(RpcRequestTest, TestFromJson)
    {
        nlohmann::json json = {
            {"jsonrpc", "2.0"},
            {"method", "getblock"},
            {"params", nlohmann::json::array({"0x123", true})},
            {"id", 42}
        };
        
        RpcRequest request = RpcRequest::FromJson(json);
        
        EXPECT_EQ("2.0", request.GetJsonRpc());
        EXPECT_EQ("getblock", request.GetMethod());
        EXPECT_EQ(json["params"], request.GetParams());
        EXPECT_EQ(42, request.GetId());
    }

    TEST_F(RpcRequestTest, TestFromJsonPartial)
    {
        // Test with missing optional fields
        nlohmann::json json = {
            {"method", "getversion"}
        };
        
        RpcRequest request = RpcRequest::FromJson(json);
        
        EXPECT_EQ("2.0", request.GetJsonRpc()); // Should use default
        EXPECT_EQ("getversion", request.GetMethod());
        EXPECT_TRUE(request.GetParams().is_null());
        EXPECT_TRUE(request.GetId().is_null());
    }

    TEST_F(RpcRequestTest, TestFromJsonEmpty)
    {
        nlohmann::json json = {};
        
        RpcRequest request = RpcRequest::FromJson(json);
        
        EXPECT_EQ("2.0", request.GetJsonRpc()); // Should use default
        EXPECT_TRUE(request.GetMethod().empty());
        EXPECT_TRUE(request.GetParams().is_null());
        EXPECT_TRUE(request.GetId().is_null());
    }

    TEST_F(RpcRequestTest, TestRoundTrip)
    {
        // Test serialization round trip
        nlohmann::json original_params = {
            {"hash", "0x1234567890abcdef"},
            {"verbose", true},
            {"index", 123}
        };
        
        RpcRequest original("2.0", "getblock", original_params, "test_id");
        
        // Serialize to JSON
        nlohmann::json json = original.ToJson();
        
        // Deserialize back
        RpcRequest deserialized = RpcRequest::FromJson(json);
        
        // Verify all fields match
        EXPECT_EQ(original.GetJsonRpc(), deserialized.GetJsonRpc());
        EXPECT_EQ(original.GetMethod(), deserialized.GetMethod());
        EXPECT_EQ(original.GetParams(), deserialized.GetParams());
        EXPECT_EQ(original.GetId(), deserialized.GetId());
    }

    TEST_F(RpcRequestTest, TestDifferentParamTypes)
    {
        // Test with array parameters
        nlohmann::json array_params = nlohmann::json::array({"param1", 42, true});
        RpcRequest request1("2.0", "method1", array_params, 1);
        
        auto json1 = request1.ToJson();
        auto deserialized1 = RpcRequest::FromJson(json1);
        EXPECT_EQ(array_params, deserialized1.GetParams());
        
        // Test with object parameters
        nlohmann::json object_params = {
            {"key1", "value1"},
            {"key2", 42},
            {"key3", true}
        };
        RpcRequest request2("2.0", "method2", object_params, 2);
        
        auto json2 = request2.ToJson();
        auto deserialized2 = RpcRequest::FromJson(json2);
        EXPECT_EQ(object_params, deserialized2.GetParams());
        
        // Test with null parameters
        RpcRequest request3("2.0", "method3", nlohmann::json(), 3);
        
        auto json3 = request3.ToJson();
        auto deserialized3 = RpcRequest::FromJson(json3);
        EXPECT_TRUE(deserialized3.GetParams().is_null());
    }

    TEST_F(RpcRequestTest, TestDifferentIdTypes)
    {
        // Test with string ID
        RpcRequest request1("2.0", "method", nlohmann::json::array(), "string_id");
        auto json1 = request1.ToJson();
        auto deserialized1 = RpcRequest::FromJson(json1);
        EXPECT_EQ("string_id", deserialized1.GetId());
        
        // Test with number ID
        RpcRequest request2("2.0", "method", nlohmann::json::array(), 42);
        auto json2 = request2.ToJson();
        auto deserialized2 = RpcRequest::FromJson(json2);
        EXPECT_EQ(42, deserialized2.GetId());
        
        // Test with null ID
        RpcRequest request3("2.0", "method", nlohmann::json::array(), nlohmann::json());
        auto json3 = request3.ToJson();
        auto deserialized3 = RpcRequest::FromJson(json3);
        EXPECT_TRUE(deserialized3.GetId().is_null());
    }

    TEST_F(RpcRequestTest, TestCommonRpcMethods)
    {
        // Test common Neo RPC methods
        std::vector<std::string> common_methods = {
            "getversion",
            "getblockcount",
            "getbestblockhash",
            "getblock",
            "getrawtransaction",
            "sendrawtransaction",
            "invokefunction",
            "invokescript",
            "getcontractstate",
            "getstorage",
            "getapplicationlog"
        };
        
        for (size_t i = 0; i < common_methods.size(); ++i)
        {
            RpcRequest request("2.0", common_methods[i], nlohmann::json::array(), static_cast<int>(i));
            
            auto json = request.ToJson();
            auto deserialized = RpcRequest::FromJson(json);
            
            EXPECT_EQ(common_methods[i], deserialized.GetMethod());
            EXPECT_EQ(static_cast<int>(i), deserialized.GetId());
        }
    }

    TEST_F(RpcRequestTest, TestInvalidJson)
    {
        // Test with invalid JSON types
        nlohmann::json invalid_json = {
            {"jsonrpc", 123}, // Should be string
            {"method", true}, // Should be string
            {"params", "invalid"}, // Should be array or object
            {"id", nlohmann::json::object()} // Should be string, number, or null
        };
        
        // Should not throw, but should handle gracefully
        EXPECT_NO_THROW({
            RpcRequest request = RpcRequest::FromJson(invalid_json);
        });
    }

    TEST_F(RpcRequestTest, TestLargeRequest)
    {
        // Test with large parameters
        nlohmann::json large_params = nlohmann::json::array();
        for (int i = 0; i < 1000; ++i)
        {
            large_params.push_back("param_" + std::to_string(i));
        }
        
        RpcRequest request("2.0", "large_method", large_params, 999);
        
        auto json = request.ToJson();
        auto deserialized = RpcRequest::FromJson(json);
        
        EXPECT_EQ("large_method", deserialized.GetMethod());
        EXPECT_EQ(1000, deserialized.GetParams().size());
        EXPECT_EQ(999, deserialized.GetId());
    }
}
