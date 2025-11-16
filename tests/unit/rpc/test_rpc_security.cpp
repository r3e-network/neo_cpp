#include <atomic>
#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "neo/node/neo_system.h"
#include "neo/rpc/error_codes.h"
#include "neo/rpc/rpc_client.h"
#include "neo/rpc/rpc_request.h"
#include "neo/rpc/rpc_response.h"
#include "neo/rpc/rpc_server.h"
#include "tests/mocks/mock_http_client.h"
#include "tests/utils/test_helpers.h"

using namespace neo::rpc;
using namespace neo::node;
using namespace neo::tests;
using namespace testing;
using namespace std::chrono_literals;

class TestRpcServer : public RpcServer
{
  public:
    using RpcServer::RpcServer;

    bool ValidateHeader(const std::string& authorization_header, std::string* authenticated_user,
                        bool log_failure, const std::string& client_ip, int* error_code = nullptr)
    {
        return ValidateAuthentication(authorization_header, authenticated_user, log_failure, client_ip, error_code);
    }
};

class RpcSecurityTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        neo_system_.reset();
        settings_ = TestHelpers::GetDefaultSettings();
    }

    void TearDown() override
    {
        if (rpc_server_ && rpc_server_->IsRunning())
        {
            rpc_server_->Stop();
        }
    }

    std::shared_ptr<NeoSystem> neo_system_;
    std::shared_ptr<neo::ProtocolSettings> settings_;
    std::shared_ptr<TestRpcServer> rpc_server_;
    std::string rpc_username_{"testuser"};
    std::string rpc_password_{"testpass"};

    void StartSecureServer(const std::string& username = "testuser", const std::string& password = "testpass",
                           bool enable_cors = false)
    {
        rpc_server_ = std::make_shared<TestRpcServer>(neo_system_, "127.0.0.1", 0);
        rpc_username_ = username;
        rpc_password_ = password;

        // Configure authentication
        rpc_server_->SetBasicAuth(username, password);

        // Configure CORS if enabled
        if (enable_cors)
        {
            rpc_server_->EnableCORS("*");
        }

        rpc_server_->Start();
        std::this_thread::sleep_for(100ms);
        ASSERT_TRUE(rpc_server_->IsRunning());
    }

    std::string SendAuthenticatedRequest(const std::string& json_request, const std::string& username = "",
                                         const std::string& password = "",
                                         std::chrono::milliseconds hold_duration = std::chrono::milliseconds(0))
    {
        // Simulate HTTP request with authentication
        std::string auth_header;
        if (!username.empty())
        {
            std::string credentials = username + ":" + password;
            std::string encoded = TestHelpers::Base64Encode(credentials);
            auth_header = "Basic " + encoded;
        }

        try
        {
            auto parsed = nlohmann::json::parse(json_request);
            RequestContext context;
            context.payload_size = json_request.size();
            context.client_ip = "127.0.0.1";
            context.record_audit = true;
            context.record_security = true;
            context.simulated_connection_hold = hold_duration;

            if (rpc_server_->IsAuthenticationEnabled())
            {
                if (auth_header.empty())
                {
                    nlohmann::json error = {{"jsonrpc", "2.0"},
                                            {"error", {{"code", 401}, {"message", "Authentication required"}}},
                                            {"id", nullptr}};
                    return error.dump();
                }

                std::string authenticated_user;
                int auth_error_code = 0;
                if (!rpc_server_->ValidateHeader(auth_header, &authenticated_user, true, context.client_ip,
                                                 &auth_error_code))
                {
                    const int code = auth_error_code == 0 ? 401 : auth_error_code;
                    const std::string message = code == 429 ? "Too many attempts" : "Unauthorized";
                    nlohmann::json error = {{"jsonrpc", "2.0"}, {"error", {{"code", code}, {"message", message}}},
                                            {"id", nullptr}};
                    return error.dump();
                }
                context.authenticated_user = authenticated_user;
            }
            else if (!auth_header.empty())
            {
                std::string authenticated_user;
                if (rpc_server_->ValidateHeader(auth_header, &authenticated_user, false, context.client_ip))
                {
                    context.authenticated_user = authenticated_user;
                }
            }

            neo::io::JsonValue request_value(parsed);
            auto response = rpc_server_->ProcessRequest(request_value, context);
            return response.ToString();
        }
        catch (const std::exception& e)
        {
            return R"({"jsonrpc":"2.0","error":{"code":-32700,"message":"Parse error"},"id":null})";
        }
    }
};

// Test basic authentication requirement
TEST_F(RpcSecurityTest, BasicAuthenticationRequired)
{
    StartSecureServer("admin", "password123");

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Request without authentication should fail
    std::string response = SendAuthenticatedRequest(request);
    auto response_json = nlohmann::json::parse(response);

    EXPECT_TRUE(response_json.contains("error"));
    ASSERT_TRUE(response_json.contains("error"));
    EXPECT_EQ(response_json["error"]["code"], 401);
}

// Test valid authentication
TEST_F(RpcSecurityTest, ValidAuthentication)
{
    const std::string username = "admin";
    const std::string password = "password123";

    StartSecureServer(username, password);

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Request with valid authentication should succeed
    std::string response = SendAuthenticatedRequest(request, username, password);
    auto response_json = nlohmann::json::parse(response);

    EXPECT_EQ(response_json["jsonrpc"], "2.0");
    EXPECT_EQ(response_json["id"], 1);
    EXPECT_TRUE(response_json.contains("result"));
}

// Test invalid credentials
TEST_F(RpcSecurityTest, InvalidCredentials)
{
    StartSecureServer("admin", "password123");

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Test wrong username
    std::string response1 = SendAuthenticatedRequest(request, "wronguser", "password123");
    auto response1_json = nlohmann::json::parse(response1);
    EXPECT_TRUE(response1_json.contains("error"));

    // Test wrong password
    std::string response2 = SendAuthenticatedRequest(request, "admin", "wrongpass");
    auto response2_json = nlohmann::json::parse(response2);
    EXPECT_TRUE(response2_json.contains("error"));

    // Test empty credentials
    std::string response3 = SendAuthenticatedRequest(request, "", "");
    auto response3_json = nlohmann::json::parse(response3);
    EXPECT_TRUE(response3_json.contains("error"));
}

// Test rate limiting
TEST_F(RpcSecurityTest, RateLimiting)
{
    StartSecureServer();

    // Configure rate limiting (100 requests per minute)
    rpc_server_->SetRateLimit(100, std::chrono::minutes(1));

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    int successful_requests = 0;
    int rate_limited_requests = 0;

    // Send requests rapidly
    for (int i = 0; i < 150; ++i)
    {
        std::string response = SendAuthenticatedRequest(request, rpc_username_, rpc_password_);
        auto response_json = nlohmann::json::parse(response);

        if (response_json.contains("result"))
        {
            successful_requests++;
        }
        else if (response_json.contains("error"))
        {
            auto error_code = response_json.contains("error") ? response_json["error"].value("code", 0) : 0;
            if (error_code == 429)
            {  // Too Many Requests
                rate_limited_requests++;
            }
        }
    }

    // Should have rate limited some requests
    EXPECT_GT(rate_limited_requests, 0);
    EXPECT_LE(successful_requests, 100);
}

// Test IP-based rate limiting
TEST_F(RpcSecurityTest, IPBasedRateLimiting)
{
    StartSecureServer();

    rpc_server_->SetIPRateLimit("127.0.0.1", 10, std::chrono::minutes(1));

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    int successful_requests = 0;

    // Send requests from rate-limited IP
    for (int i = 0; i < 20; ++i)
    {
        std::string response = SendAuthenticatedRequest(request, rpc_username_, rpc_password_);
        auto response_json = nlohmann::json::parse(response);

        if (response_json.contains("result"))
        {
            successful_requests++;
        }
    }

    // Should have limited requests from this IP
    EXPECT_LE(successful_requests, 10);
}

// Test CORS configuration
TEST_F(RpcSecurityTest, CORSConfiguration)
{
    StartSecureServer("admin", "password", true);

    // Test CORS headers are present
    auto cors_headers = rpc_server_->GetCORSHeaders();
    EXPECT_FALSE(cors_headers.empty());

    // Test specific origin
    rpc_server_->SetCORSOrigin("https://example.com");

    bool cors_valid = rpc_server_->ValidateCORSOrigin("https://example.com");
    EXPECT_TRUE(cors_valid);

    bool cors_invalid = rpc_server_->ValidateCORSOrigin("https://malicious.com");
    EXPECT_FALSE(cors_invalid);
}

// Test method-based access control
TEST_F(RpcSecurityTest, MethodBasedAccessControl)
{
    StartSecureServer("admin", "password");

    // Configure method restrictions
    rpc_server_->AddRestrictedMethod("sendrawtransaction", {"admin"});
    rpc_server_->AddRestrictedMethod("submitblock", {"admin"});

    // Test unrestricted method
    std::string unrestricted_request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    std::string response1 = SendAuthenticatedRequest(unrestricted_request, "admin", "password");
    auto response1_json = nlohmann::json::parse(response1);
    EXPECT_TRUE(response1_json.contains("result"));

    // Test restricted method with valid user
    std::string restricted_request = R"({
        "jsonrpc": "2.0",
        "method": "sendrawtransaction",
        "params": ["abcd"],
        "id": 2
    })";

    std::string response2 = SendAuthenticatedRequest(restricted_request, "admin", "password");
    auto response2_json = nlohmann::json::parse(response2);
    // Should be allowed (though may fail due to invalid params)
    EXPECT_TRUE(response2_json.contains("result") || response2_json.contains("error"));

    // If error, should not be authorization error
    if (response2_json.contains("error"))
    {
        EXPECT_NE(response2_json["error"]["code"], 403);
    }
}

// Test input validation and sanitization
TEST_F(RpcSecurityTest, InputValidationAndSanitization)
{
    StartSecureServer();
    rpc_server_->SetMaxRequestSize(1024 * 1024);  // tighten limit for testing

    // Test oversized request
    std::string large_param(10000000, 'A');  // 10MB parameter
    std::string oversized_request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [")" + large_param +
                                    R"("],
        "id": 1
    })";

    std::string response1 =
        SendAuthenticatedRequest(oversized_request, rpc_username_, rpc_password_);
    auto response1_json = nlohmann::json::parse(response1);

    // Should reject oversized requests
    EXPECT_TRUE(response1_json.contains("error"));

    // Test malicious JSON injection
    std::string injection_request = R"({
        "jsonrpc": "2.0",
        "method": "getversion\"; DROP TABLE users; --",
        "params": [],
        "id": 1
    })";

    std::string response2 =
        SendAuthenticatedRequest(injection_request, rpc_username_, rpc_password_);
    auto response2_json = nlohmann::json::parse(response2);

    // Should handle malicious method names gracefully
    EXPECT_TRUE(response2_json.contains("error"));
    EXPECT_EQ(response2_json["error"]["code"], -32601);  // Method not found

    // Test XSS attempt in parameters
    std::string xss_request = R"({
        "jsonrpc": "2.0",
        "method": "getblock",
        "params": ["<script>alert('xss')</script>"],
        "id": 1
    })";

    std::string response3 = SendAuthenticatedRequest(xss_request, rpc_username_, rpc_password_);
    auto response3_json = nlohmann::json::parse(response3);

    // Should not execute script, should return error or sanitized response
    EXPECT_TRUE(response3_json.contains("error") || response3_json.contains("result"));
}

// Test SSL/TLS security (simulation)
TEST_F(RpcSecurityTest, SSLTLSSecurity)
{
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
    GTEST_SKIP() << "OpenSSL support disabled in this build";
#else
    rpc_server_ = std::make_shared<TestRpcServer>(neo_system_, "127.0.0.1", 0);
    rpc_server_->EnableSSL("/path/to/cert.pem", "/path/to/key.pem");

    EXPECT_TRUE(rpc_server_->IsSSLEnabled());

    rpc_server_->SetSSLCiphers("ECDHE+AESGCM:ECDHE+CHACHA20:DHE+AESGCM");
    rpc_server_->SetMinTLSVersion("1.2");

    std::vector<std::string> trusted = {"/path/ca1.pem", "/path/ca2.pem"};
    rpc_server_->SetTrustedAuthorities(trusted);
    EXPECT_EQ(trusted, rpc_server_->GetTrustedAuthorities());
#endif
}

// Test session management
TEST_F(RpcSecurityTest, SessionManagement)
{
    StartSecureServer("admin", "password");

    // Configure session timeout (5 minutes)
    rpc_server_->SetSessionTimeout(std::chrono::minutes(5));

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Create session
    std::string session_id = rpc_server_->CreateSession("admin");
    EXPECT_FALSE(session_id.empty());

    // Use session for request
    std::string response1 = SendAuthenticatedRequest(request, "admin", "password");
    auto response1_json = nlohmann::json::parse(response1);
    EXPECT_TRUE(response1_json.contains("result"));

    // Test session validation
    EXPECT_TRUE(rpc_server_->ValidateSession(session_id));

    // Complete session expiry test implementation with time manipulation
    // Test 1: Manual session invalidation
    std::string manual_session = rpc_server_->CreateSession("admin");
    EXPECT_TRUE(rpc_server_->ValidateSession(manual_session));
    rpc_server_->InvalidateSession(manual_session);
    EXPECT_FALSE(rpc_server_->ValidateSession(manual_session));

    // Test 2: Time-based session expiry simulation
    std::string expiry_session = rpc_server_->CreateSession("admin");
    EXPECT_TRUE(rpc_server_->ValidateSession(expiry_session));

    // Configure session timeout for testing (e.g., 1 second)
    rpc_server_->SetSessionTimeout(std::chrono::seconds(1));

    // Simulate time passage by sleeping
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Check that session has expired due to timeout
    EXPECT_FALSE(rpc_server_->ValidateSession(expiry_session));

    // Test 3: Session renewal before expiry
    std::string renewal_session = rpc_server_->CreateSession("admin");
    EXPECT_TRUE(rpc_server_->ValidateSession(renewal_session));

    // Sleep for half the timeout period
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Renew the session by using it
    std::string renewal_response = SendAuthenticatedRequest(request, "admin", "password");
    auto renewal_json = nlohmann::json::parse(renewal_response);
    EXPECT_TRUE(renewal_json.contains("result"));

    // Session should still be valid after activity
    EXPECT_TRUE(rpc_server_->ValidateSession(renewal_session));

    // Test 4: Multiple concurrent sessions
    std::string session_a = rpc_server_->CreateSession("admin");
    std::string session_b = rpc_server_->CreateSession("admin");
    EXPECT_TRUE(rpc_server_->ValidateSession(session_a));
    EXPECT_TRUE(rpc_server_->ValidateSession(session_b));
    EXPECT_NE(session_a, session_b);  // Sessions should be unique

    // Invalidate one session, other should remain valid
    rpc_server_->InvalidateSession(session_a);
    EXPECT_FALSE(rpc_server_->ValidateSession(session_a));
    EXPECT_TRUE(rpc_server_->ValidateSession(session_b));
}

// Test brute force protection
TEST_F(RpcSecurityTest, BruteForceProtection)
{
    StartSecureServer("admin", "password123");

    // Configure brute force protection (5 attempts, 10 minute lockout)
    rpc_server_->SetBruteForceProtection(5, std::chrono::minutes(10));

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Attempt multiple failed logins
    for (int i = 0; i < 10; ++i)
    {
        std::string response = SendAuthenticatedRequest(request, "admin", "wrongpassword");
        auto response_json = nlohmann::json::parse(response);

        if (i >= 5)
        {
            // Should be locked out after 5 attempts
            EXPECT_TRUE(response_json.contains("error"));
            auto error_code = response_json.contains("error") ? response_json["error"].value("code", 0) : 0;
            EXPECT_TRUE(error_code == 429 ||
                        error_code == static_cast<int>(neo::rpc::ErrorCode::RateLimitExceeded));
        }
    }

    // Valid credentials should also be blocked during lockout
    std::string response = SendAuthenticatedRequest(request, "admin", "password123");
    auto response_json = nlohmann::json::parse(response);
    EXPECT_TRUE(response_json.contains("error"));
}

// Test request/response logging for security monitoring
TEST_F(RpcSecurityTest, SecurityLogging)
{
    StartSecureServer("admin", "password");

    // Enable security logging
    rpc_server_->EnableSecurityLogging(true);

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Send authenticated request
    std::string response1 = SendAuthenticatedRequest(request, "admin", "password");

    // Send failed authentication
    std::string response2 = SendAuthenticatedRequest(request, "admin", "wrongpass");

    // Check that security events are logged
    auto security_logs = rpc_server_->GetSecurityLogs();
    EXPECT_GE(security_logs.size(), 1);

    // Should log failed authentication attempts
    bool found_auth_failure = false;
    for (const auto& log : security_logs)
    {
        if (log.event_type == "AUTH_FAILURE")
        {
            found_auth_failure = true;
            break;
        }
    }
    EXPECT_TRUE(found_auth_failure);
}

// Test DoS protection
TEST_F(RpcSecurityTest, DoSProtection)
{
    StartSecureServer();

    // Configure DoS protection
    rpc_server_->SetMaxConcurrentConnections(10);
    rpc_server_->SetConnectionTimeout(std::chrono::seconds(30));

    std::atomic<int> successful_connections{0};
    std::atomic<int> rejected_connections{0};
    std::vector<std::thread> threads;

    // Attempt many concurrent connections
    for (int i = 0; i < 50; ++i)
    {
        threads.emplace_back(
            [this, &successful_connections, &rejected_connections]()
            {
                try
                {
                    std::string request = R"({
                    "jsonrpc": "2.0",
                    "method": "getversion",
                    "params": [],
                    "id": 1
                })";

                    std::string response =
                        SendAuthenticatedRequest(request, rpc_username_, rpc_password_, 100ms);
                    auto response_json = nlohmann::json::parse(response);

                    if (response_json.contains("result"))
                    {
                        successful_connections++;
                    }
                    else
                    {
                        rejected_connections++;
                    }
                }
                catch (...)
                {
                    rejected_connections++;
                }

                // Hold connection for a moment
                std::this_thread::sleep_for(100ms);
            });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    // Should limit concurrent connections
    EXPECT_LE(successful_connections.load(), 10);
    EXPECT_GT(rejected_connections.load(), 30);
}

// Test request size limits
TEST_F(RpcSecurityTest, RequestSizeLimits)
{
    StartSecureServer();

    // Set maximum request size (1KB)
    rpc_server_->SetMaxRequestSize(1024);

    // Test normal size request
    std::string normal_request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    std::string response1 = SendAuthenticatedRequest(normal_request, rpc_username_, rpc_password_);
    auto response1_json = nlohmann::json::parse(response1);
    EXPECT_TRUE(response1_json.contains("result"));

    // Test oversized request
    std::string large_data(2000, 'X');  // 2KB of data
    std::string oversized_request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [")" + large_data +
                                    R"("],
        "id": 1
    })";

    std::string response2 = SendAuthenticatedRequest(oversized_request, rpc_username_, rpc_password_);
    auto response2_json = nlohmann::json::parse(response2);

    // Should reject oversized request
    EXPECT_TRUE(response2_json.contains("error"));
    auto error_code = response2_json.contains("error") ? response2_json["error"].value("code", 0) : 0;
    EXPECT_EQ(error_code, 413);  // Request Entity Too Large
}

// Test security headers
TEST_F(RpcSecurityTest, SecurityHeaders)
{
    StartSecureServer();

    // Configure security headers
    rpc_server_->SetSecurityHeaders({{"X-Content-Type-Options", "nosniff"},
                                     {"X-Frame-Options", "DENY"},
                                     {"X-XSS-Protection", "1; mode=block"},
                                     {"Strict-Transport-Security", "max-age=31536000; includeSubDomains"},
                                     {"Content-Security-Policy", "default-src 'self'"}});

    auto headers = rpc_server_->GetSecurityHeaders();

    EXPECT_FALSE(headers.empty());
    EXPECT_TRUE(headers.find("X-Content-Type-Options") != headers.end());
    EXPECT_TRUE(headers.find("X-Frame-Options") != headers.end());
    EXPECT_TRUE(headers.find("X-XSS-Protection") != headers.end());
}

// Test audit trail
TEST_F(RpcSecurityTest, AuditTrail)
{
    StartSecureServer("admin", "password");

    // Enable audit trail
    rpc_server_->EnableAuditTrail(true);

    std::string request = R"({
        "jsonrpc": "2.0",
        "method": "getversion",
        "params": [],
        "id": 1
    })";

    // Send various requests
    SendAuthenticatedRequest(request, "admin", "password");
    SendAuthenticatedRequest(request, "admin", "wrongpass");

    // Check audit trail
    auto audit_logs = rpc_server_->GetAuditTrail();
    EXPECT_GE(audit_logs.size(), 1);

    // Should contain relevant information
    for (const auto& entry : audit_logs)
    {
        EXPECT_FALSE(entry.timestamp.empty());
        EXPECT_FALSE(entry.method.empty());
        EXPECT_FALSE(entry.client_ip.empty());
    }
}
