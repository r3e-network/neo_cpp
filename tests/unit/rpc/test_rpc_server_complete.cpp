#include <gtest/gtest.h>
#include <neo/rpc/rpc_server.h>
#include <neo/rpc/rpc_request.h>
#include <neo/rpc/rpc_response.h>
#include <neo/rpc/rpc_error.h>
#include <neo/rpc/rpc_methods.h>
#include <neo/core/neo_system.h>
#include <neo/ledger/blockchain.h>
#include <neo/network/p2p/local_node.h>
#include <nlohmann/json.hpp>

using namespace neo::rpc;
using namespace neo::core;
using namespace neo::ledger;
using json = nlohmann::json;

namespace neo::rpc::tests
{

class RpcServerCompleteTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test blockchain and neo system
        mock_system = std::make_shared<NeoSystem>();
        rpc_server = std::make_unique<RpcServer>(mock_system);
        
        // Setup test configuration
        config["port"] = 10331;
        config["bind"] = "127.0.0.1";
        config["username"] = "test";
        config["password"] = "test123";
        config["cors"] = true;
        config["maxconnections"] = 40;
        
        rpc_server->Configure(config);
    }

    void TearDown() override
    {
        if (rpc_server && rpc_server->IsRunning()) {
            rpc_server->Stop();
        }
    }

    std::shared_ptr<NeoSystem> mock_system;
    std::unique_ptr<RpcServer> rpc_server;
    json config;
};

// Blockchain RPC Methods Tests
TEST_F(RpcServerCompleteTest, GetBestBlockHash)
{
    // Test: getbestblockhash RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getbestblockhash"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_string());
    EXPECT_EQ(response_json["result"].get<std::string>().length(), 66); // 0x + 64 hex chars
}

TEST_F(RpcServerCompleteTest, GetBlock)
{
    // Test: getblock RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getblock"},
        {"params", {"0x0000000000000000000000000000000000000000000000000000000000000000", true}},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    
    if (response_json.contains("result")) {
        // Block found
        EXPECT_TRUE(response_json["result"].is_object());
        EXPECT_TRUE(response_json["result"].contains("hash"));
        EXPECT_TRUE(response_json["result"].contains("size"));
        EXPECT_TRUE(response_json["result"].contains("version"));
        EXPECT_TRUE(response_json["result"].contains("previousblockhash"));
        EXPECT_TRUE(response_json["result"].contains("merkleroot"));
        EXPECT_TRUE(response_json["result"].contains("time"));
        EXPECT_TRUE(response_json["result"].contains("index"));
        EXPECT_TRUE(response_json["result"].contains("nextconsensus"));
        EXPECT_TRUE(response_json["result"].contains("witnesses"));
        EXPECT_TRUE(response_json["result"].contains("tx"));
    } else {
        // Block not found - should return error
        EXPECT_TRUE(response_json.contains("error"));
        EXPECT_EQ(response_json["error"]["code"], -100); // Unknown block
    }
}

TEST_F(RpcServerCompleteTest, GetBlockCount)
{
    // Test: getblockcount RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getblockcount"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_number_unsigned());
    EXPECT_GE(response_json["result"].get<uint32_t>(), 0);
}

TEST_F(RpcServerCompleteTest, GetBlockHeader)
{
    // Test: getblockheader RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getblockheader"},
        {"params", {"0x0000000000000000000000000000000000000000000000000000000000000000", true}},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    
    if (response_json.contains("result")) {
        // Header found
        EXPECT_TRUE(response_json["result"].is_object());
        EXPECT_TRUE(response_json["result"].contains("hash"));
        EXPECT_TRUE(response_json["result"].contains("size"));
        EXPECT_TRUE(response_json["result"].contains("version"));
        EXPECT_TRUE(response_json["result"].contains("previousblockhash"));
        EXPECT_TRUE(response_json["result"].contains("merkleroot"));
        EXPECT_TRUE(response_json["result"].contains("time"));
        EXPECT_TRUE(response_json["result"].contains("index"));
        EXPECT_TRUE(response_json["result"].contains("nextconsensus"));
        EXPECT_TRUE(response_json["result"].contains("witnesses"));
        // Header should NOT contain "tx" field
        EXPECT_FALSE(response_json["result"].contains("tx"));
    }
}

TEST_F(RpcServerCompleteTest, GetRawMemPool)
{
    // Test: getrawmempool RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getrawmempool"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_array());
}

// Node RPC Methods Tests
TEST_F(RpcServerCompleteTest, GetConnectionCount)
{
    // Test: getconnectioncount RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getconnectioncount"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_number_unsigned());
    EXPECT_GE(response_json["result"].get<uint32_t>(), 0);
}

TEST_F(RpcServerCompleteTest, GetPeers)
{
    // Test: getpeers RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getpeers"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_object());
    EXPECT_TRUE(response_json["result"].contains("unconnected"));
    EXPECT_TRUE(response_json["result"].contains("bad"));
    EXPECT_TRUE(response_json["result"].contains("connected"));
}

TEST_F(RpcServerCompleteTest, GetVersion)
{
    // Test: getversion RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getversion"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_object());
    EXPECT_TRUE(response_json["result"].contains("tcpport"));
    EXPECT_TRUE(response_json["result"].contains("wsport"));
    EXPECT_TRUE(response_json["result"].contains("nonce"));
    EXPECT_TRUE(response_json["result"].contains("useragent"));
}

// Smart Contract RPC Methods Tests
TEST_F(RpcServerCompleteTest, InvokeFunction)
{
    // Test: invokefunction RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "invokefunction"},
        {"params", {
            "0xef4073a0f2b305a38ec4050e4d3d28bc40ea63f5", // NEO token hash
            "symbol",
            json::array()
        }},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_object());
    EXPECT_TRUE(response_json["result"].contains("script"));
    EXPECT_TRUE(response_json["result"].contains("state"));
    EXPECT_TRUE(response_json["result"].contains("gasconsumed"));
    EXPECT_TRUE(response_json["result"].contains("stack"));
}

TEST_F(RpcServerCompleteTest, InvokeScript)
{
    // Test: invokescript RPC method
    std::string script = "0c0474657374"; // Simple test script
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "invokescript"},
        {"params", {script}},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_object());
    EXPECT_TRUE(response_json["result"].contains("script"));
    EXPECT_TRUE(response_json["result"].contains("state"));
    EXPECT_TRUE(response_json["result"].contains("gasconsumed"));
    EXPECT_TRUE(response_json["result"].contains("stack"));
}

// Utilities RPC Methods Tests
TEST_F(RpcServerCompleteTest, ValidateAddress)
{
    // Test: validateaddress RPC method
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "validateaddress"},
        {"params", {"NfgHwwTi3wHAS8aFAN243C5vGbkYDpqLHP"}}, // Valid Neo address
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_TRUE(response_json["result"].is_object());
    EXPECT_TRUE(response_json["result"].contains("address"));
    EXPECT_TRUE(response_json["result"].contains("isvalid"));
}

// Error Handling Tests
TEST_F(RpcServerCompleteTest, InvalidMethod)
{
    // Test: Invalid method should return error
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "invalidmethod"},
        {"params", json::array()},
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32601); // Method not found
}

TEST_F(RpcServerCompleteTest, InvalidParameters)
{
    // Test: Invalid parameters should return error
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getblock"},
        {"params", {"invalid_hash"}}, // Invalid block hash
        {"id", 1}
    };
    
    auto response = rpc_server->ProcessRequest(request.dump());
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32602); // Invalid params
}

TEST_F(RpcServerCompleteTest, MalformedRequest)
{
    // Test: Malformed JSON should return error
    std::string malformed_json = "{\"jsonrpc\":\"2.0\",\"method\":\"getblock\",";
    
    auto response = rpc_server->ProcessRequest(malformed_json);
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32700); // Parse error
}

// Authentication Tests
TEST_F(RpcServerCompleteTest, AuthenticationRequired)
{
    // Test: Authentication when required
    rpc_server->EnableAuthentication(true);
    
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getbestblockhash"},
        {"params", json::array()},
        {"id", 1}
    };
    
    // Request without authentication should fail
    auto response = rpc_server->ProcessRequestWithAuth(request.dump(), "", "");
    EXPECT_TRUE(response.has_value());
    
    auto response_json = json::parse(response.value());
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32600); // Invalid request (auth required)
    
    // Request with valid authentication should succeed
    response = rpc_server->ProcessRequestWithAuth(request.dump(), "test", "test123");
    EXPECT_TRUE(response.has_value());
    
    response_json = json::parse(response.value());
    EXPECT_TRUE(response_json.contains("result"));
}

// Performance Tests
TEST_F(RpcServerCompleteTest, ConcurrentRequests)
{
    // Test: Handle multiple concurrent requests
    const int num_requests = 100;
    std::vector<std::future<std::optional<std::string>>> futures;
    
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getblockcount"},
        {"params", json::array()},
        {"id", 1}
    };
    
    // Submit concurrent requests
    for (int i = 0; i < num_requests; i++) {
        auto future = std::async(std::launch::async, [this, request]() {
            return rpc_server->ProcessRequest(request.dump());
        });
        futures.push_back(std::move(future));
    }
    
    // Verify all requests complete successfully
    int successful_requests = 0;
    for (auto& future : futures) {
        auto response = future.get();
        if (response.has_value()) {
            auto response_json = json::parse(response.value());
            if (response_json.contains("result")) {
                successful_requests++;
            }
        }
    }
    
    EXPECT_EQ(successful_requests, num_requests);
}

TEST_F(RpcServerCompleteTest, RateLimiting)
{
    // Test: Rate limiting functionality
    rpc_server->EnableRateLimit(true, 10, std::chrono::seconds(1)); // 10 requests per second
    
    json request = {
        {"jsonrpc", "2.0"},
        {"method", "getblockcount"},
        {"params", json::array()},
        {"id", 1}
    };
    
    // Send requests rapidly
    int successful_requests = 0;
    int rate_limited_requests = 0;
    
    for (int i = 0; i < 20; i++) {
        auto response = rpc_server->ProcessRequest(request.dump());
        if (response.has_value()) {
            auto response_json = json::parse(response.value());
            if (response_json.contains("result")) {
                successful_requests++;
            } else if (response_json.contains("error")) {
                auto error_code = response_json["error"]["code"];
                if (error_code == -32429) { // Too many requests
                    rate_limited_requests++;
                }
            }
        }
    }
    
    // Should have some successful requests and some rate limited
    EXPECT_GT(successful_requests, 0);
    EXPECT_GT(rate_limited_requests, 0);
    EXPECT_EQ(successful_requests + rate_limited_requests, 20);
}

}  // namespace neo::rpc::tests