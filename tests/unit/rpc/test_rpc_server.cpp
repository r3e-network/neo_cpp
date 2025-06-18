#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <string>
#include <vector>

#include "neo/rpc/rpc_server.h"
#include "neo/rpc/rpc_request.h"
#include "neo/rpc/rpc_response.h"
#include "neo/rpc/rpc_methods.h"
#include "neo/node/neo_system.h"
#include "tests/mocks/mock_neo_system.h"
#include "tests/mocks/mock_http_client.h"
#include "tests/utils/test_helpers.h"

using namespace neo::rpc;
using namespace neo::node;
using namespace neo::tests;
using namespace testing;
using namespace std::chrono_literals;

class RpcServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        neo_system_ = std::make_shared<MockNeoSystem>();
        settings_ = TestHelpers::GetDefaultSettings();
        
        // Setup mock expectations
        EXPECT_CALL(*neo_system_, GetSettings())
            .WillRepeatedly(Return(settings_));
        EXPECT_CALL(*neo_system_, GetBlockchain())
            .WillRepeatedly(Return(blockchain_));
    }
    
    void TearDown() override {
        if (rpc_server_ && rpc_server_->IsRunning()) {
            rpc_server_->Stop();
        }
    }
    
    std::shared_ptr<MockNeoSystem> neo_system_;
    std::shared_ptr<ProtocolSettings> settings_;
    std::shared_ptr<Blockchain> blockchain_;
    std::shared_ptr<RpcServer> rpc_server_;
    
    void StartServer(const std::string& bind_address = "127.0.0.1", uint16_t port = 0) {
        rpc_server_ = std::make_shared<RpcServer>(neo_system_, bind_address, port);
        rpc_server_->Start();
        
        // Wait for server to start
        std::this_thread::sleep_for(100ms);
        ASSERT_TRUE(rpc_server_->IsRunning());
    }
    
    std::string SendHttpRequest(const std::string& json_request) {
        if (!rpc_server_ || !rpc_server_->IsRunning()) {
            return "";
        }
        
        // In real implementation, this would use HTTP client
        // For testing, we directly call the request processor
        try {
            auto json_obj = nlohmann::json::parse(json_request);
            auto response = rpc_server_->ProcessRequest(json_obj);
            return response.dump();
        } catch (const std::exception& e) {
            return R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null})";
        }
    }
};

// Test server lifecycle management
TEST_F(RpcServerTest, ServerLifecycle) {
    // Test server creation
    rpc_server_ = std::make_shared<RpcServer>(neo_system_, "127.0.0.1", 10332);
    EXPECT_FALSE(rpc_server_->IsRunning());
    
    // Test server start
    rpc_server_->Start();
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(rpc_server_->IsRunning());
    
    // Test server stop
    rpc_server_->Stop();
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(rpc_server_->IsRunning());
    
    // Test restart
    rpc_server_->Start();
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(rpc_server_->IsRunning());
    
    rpc_server_->Stop();
}

// Test server configuration
TEST_F(RpcServerTest, ServerConfiguration) {
    // Test different bind addresses
    auto server1 = std::make_shared<RpcServer>(neo_system_, "0.0.0.0", 10332);
    auto server2 = std::make_shared<RpcServer>(neo_system_, "127.0.0.1", 10333);
    
    server1->Start();
    server2->Start();
    
    std::this_thread::sleep_for(100ms);
    
    EXPECT_TRUE(server1->IsRunning());
    EXPECT_TRUE(server2->IsRunning());
    EXPECT_NE(server1->GetPort(), server2->GetPort());
    
    server1->Stop();
    server2->Stop();
}

// Test request processing
TEST_F(RpcServerTest, RequestProcessing) {
    StartServer();
    
    // Test valid request
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";
    
    std::string response = SendHttpRequest(request);
    EXPECT_FALSE(response.empty());
    
    auto response_json = nlohmann::json::parse(response);
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
}

// Test invalid JSON handling
TEST_F(RpcServerTest, InvalidJsonHandling) {
    StartServer();
    
    // Test malformed JSON
    std::string invalid_json = "{ invalid json }";
    std::string response = SendHttpRequest(invalid_json);
    
    auto response_json = nlohmann::json::parse(response);
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32700); // Parse error
    EXPECT_EQ(response_json["id"], nullptr);
}

// Test method not found
TEST_F(RpcServerTest, MethodNotFound) {
    StartServer();
    
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "nonexistentmethod",
        "params": [],
        "id": 1
    })";
    
    std::string response = SendHttpRequest(request);
    auto response_json = nlohmann::json::parse(response);
    
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32601); // Method not found
    EXPECT_EQ(response_json["id"], 1);
}

// Test concurrent request handling
TEST_F(RpcServerTest, ConcurrentRequests) {
    StartServer();
    
    const int num_requests = 100;
    std::vector<std::future<std::string>> futures;
    
    // Launch concurrent requests
    for (int i = 0; i < num_requests; ++i) {
        futures.push_back(std::async(std::launch::async, [this, i]() {
            std::string request = R"({
                "jsonrpc": "2.0",
                "method": "getversion",
                "params": [],
                "id": )" + std::to_string(i) + "}";
            return SendHttpRequest(request);
        }));
    }
    
    // Collect responses
    int successful_responses = 0;
    for (auto& future : futures) {
        std::string response = future.get();
        if (!response.empty()) {
            auto response_json = nlohmann::json::parse(response);
            if (response_json.contains("result")) {
                successful_responses++;
            }
        }
    }
    
    // Should handle most concurrent requests successfully
    EXPECT_GE(successful_responses, num_requests * 0.9); // 90% success rate
}

// Test batch requests
TEST_F(RpcServerTest, BatchRequests) {
    StartServer();
    
    std::string batch_request = R"([
        {
            "jsonrpc": "2.0",
            "method": "getversion",
            "params": [],
            "id": 1
        },
        {
            "jsonrpc": "2.0",
            "method": "getblockcount",
            "params": [],
            "id": 2
        },
        {
            "jsonrpc": "2.0",
            "method": "getbestblockhash",
            "params": [],
            "id": 3
        }
    ])";
    
    std::string response = SendHttpRequest(batch_request);
    auto response_json = nlohmann::json::parse(response);
    
    EXPECT_TRUE(response_json.is_array());
    EXPECT_EQ(response_json.size(), 3);
    
    for (const auto& resp : response_json) {
        EXPECT_EQ(resp["jsonrpc"], "2.0");
        EXPECT_TRUE(resp.contains("result") || resp.contains("error"));
    }
}

// Test notification requests (no ID)
TEST_F(RpcServerTest, NotificationRequests) {
    StartServer();
    
    std::string notification = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": []
    })";
    
    std::string response = SendHttpRequest(notification);
    
    // Notifications should not return a response
    EXPECT_TRUE(response.empty() || response == "null");
}

// Test request timeout handling
TEST_F(RpcServerTest, RequestTimeout) {
    StartServer();
    rpc_server_->SetTimeout(std::chrono::milliseconds(100));
    
    // Simulate slow method execution
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "simulateslowmethod",
        "params": [],
        "id": 1
    })";
    
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string response = SendHttpRequest(request);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Should timeout within reasonable time
    EXPECT_LE(duration.count(), 200); // Allow some margin
}

// Test max connections limit
TEST_F(RpcServerTest, MaxConnectionsLimit) {
    StartServer();
    rpc_server_->SetMaxConnections(5);
    
    std::vector<std::thread> connections;
    std::atomic<int> successful_connections{0};
    std::atomic<int> rejected_connections{0};
    
    // Try to create more connections than allowed
    for (int i = 0; i < 10; ++i) {
        connections.emplace_back([this, &successful_connections, &rejected_connections]() {
            try {
                std::string request = R"({
                    "jsonrpc": "2.0",
                    "method": "getversion",
                    "params": [],
                    "id": 1
                })";
                
                std::string response = SendHttpRequest(request);
                if (!response.empty()) {
                    successful_connections++;
                } else {
                    rejected_connections++;
                }
            } catch (...) {
                rejected_connections++;
            }
        });
    }
    
    for (auto& thread : connections) {
        thread.join();
    }
    
    // Should accept up to max connections and reject excess
    EXPECT_LE(successful_connections.load(), 5);
    EXPECT_GT(rejected_connections.load(), 0);
}

// Test method registration and deregistration
TEST_F(RpcServerTest, MethodRegistration) {
    StartServer();
    
    // Test custom method registration
    rpc_server_->RegisterMethod("custommethod", [](const nlohmann::json& params) {
        nlohmann::json result;
        result["custom"] = "response";
        return result;
    });
    
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "custommethod",
        "params": [],
        "id": 1
    })";
    
    std::string response = SendHttpRequest(request);
    auto response_json = nlohmann::json::parse(response);
    
    EXPECT_TRUE(response_json.contains("result"));
    EXPECT_EQ(response_json["result"]["custom"], "response");
    
    // Test method deregistration
    rpc_server_->UnregisterMethod("custommethod");
    
    response = SendHttpRequest(request);
    response_json = nlohmann::json::parse(response);
    
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32601); // Method not found
}

// Test disabled methods
TEST_F(RpcServerTest, DisabledMethods) {
    StartServer();
    
    // Disable a method
    rpc_server_->AddDisabledMethod("getversion");
    
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";
    
    std::string response = SendHttpRequest(request);
    auto response_json = nlohmann::json::parse(response);
    
    EXPECT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], -32601); // Method not found
}

// Test server shutdown during request processing
TEST_F(RpcServerTest, ShutdownDuringRequest) {
    StartServer();
    
    std::atomic<bool> request_started{false};
    std::atomic<bool> request_completed{false};
    
    // Start a long-running request
    std::thread request_thread([this, &request_started, &request_completed]() {
        request_started = true;
        
        std::string request = R"({
            "jsonrpc": "2.0",
            "method": "getversion",
            "params": [],
            "id": 1
        })";
        
        std::string response = SendHttpRequest(request);
        request_completed = true;
    });
    
    // Wait for request to start
    while (!request_started) {
        std::this_thread::sleep_for(1ms);
    }
    
    // Shutdown server while request is processing
    rpc_server_->Stop();
    
    // Wait for request thread to complete
    request_thread.join();
    
    // Server should shutdown gracefully
    EXPECT_FALSE(rpc_server_->IsRunning());
}

// Test HTTP headers and content type
TEST_F(RpcServerTest, HttpHeaders) {
    StartServer();
    
    // This test would require HTTP client integration
    // For now, we test that the server accepts requests with proper content type
    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";
    
    std::string response = SendHttpRequest(request);
    EXPECT_FALSE(response.empty());
    
    // Response should be valid JSON
    auto response_json = nlohmann::json::parse(response);
    EXPECT_EQ(response_json["jsonrpc"], "2.0");
}

// Test server statistics and monitoring
TEST_F(RpcServerTest, ServerStatistics) {
    StartServer();
    
    // Process some requests
    for (int i = 0; i < 10; ++i) {
        std::string request = R"({
            "jsonrpc": "2.0",
            "method": "getversion",
            "params": [],
            "id": )" + std::to_string(i) + "}";
        SendHttpRequest(request);
    }
    
    // Check server statistics
    auto stats = rpc_server_->GetStatistics();
    EXPECT_GE(stats.requests_processed, 10);
    EXPECT_GE(stats.total_request_time.count(), 0);
    EXPECT_GE(stats.active_connections, 0);
}

// Performance test: measure request throughput
TEST_F(RpcServerTest, RequestThroughput) {
    StartServer();
    
    const int num_requests = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_requests; ++i) {
        futures.push_back(std::async(std::launch::async, [this, i]() {
            std::string request = R"({
                "jsonrpc": "2.0",
                "method": "getversion",
                "params": [],
                "id": )" + std::to_string(i) + "}";
            SendHttpRequest(request);
        }));
    }
    
    // Wait for all requests to complete
    for (auto& future : futures) {
        future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double requests_per_second = (num_requests * 1000.0) / duration.count();
    
    // Should handle at least 100 requests per second
    EXPECT_GE(requests_per_second, 100.0);
}

// Test memory usage under load
TEST_F(RpcServerTest, MemoryUsageUnderLoad) {
    StartServer();
    
    // Get initial memory usage
    size_t initial_memory = TestHelpers::GetMemoryUsage();
    
    const int num_requests = 10000;
    for (int i = 0; i < num_requests; ++i) {
        std::string request = R"({
            "jsonrpc": "2.0",
            "method": "getversion",
            "params": [],
            "id": )" + std::to_string(i) + "}";
        SendHttpRequest(request);
    }
    
    // Force garbage collection if applicable
    std::this_thread::sleep_for(100ms);
    
    size_t final_memory = TestHelpers::GetMemoryUsage();
    size_t memory_increase = final_memory - initial_memory;
    
    // Memory increase should be reasonable (less than 100MB for 10k requests)
    EXPECT_LT(memory_increase, 100 * 1024 * 1024);
}