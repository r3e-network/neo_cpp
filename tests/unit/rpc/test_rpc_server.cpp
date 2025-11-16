#include <gtest/gtest.h>

#include <memory>
#include <string>

#include <neo/cryptography/base64.h>
#include <neo/io/json.h>
#include <neo/rpc/error_codes.h>
#include <neo/rpc/rpc_server.h>

#ifdef NEO_HAS_HTTPLIB
#include <httplib.h>
#endif

#include <nlohmann/json.hpp>

namespace neo::rpc::tests
{
namespace
{
io::JsonValue MakeRequest(const std::string& method, nlohmann::json params = nlohmann::json::array(), int id = 1)
{
    nlohmann::json request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;
    request["params"] = std::move(params);
    request["id"] = id;
    return io::JsonValue(std::move(request));
}

io::JsonValue MakeRawRequest(const nlohmann::json& json) { return io::JsonValue(json); }
}  // namespace

class RpcServerUnitTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        RpcConfig config;
        config.bind_address = "127.0.0.1";
        config.port = 0;  // Avoid binding to a fixed port when tests call Start()
        server_ = std::make_unique<RpcServer>(config, std::shared_ptr<node::NeoSystem>{});
    }

    std::unique_ptr<RpcServer> server_;
};

TEST_F(RpcServerUnitTest, RejectsRequestsMissingMethod)
{
    nlohmann::json without_method = {{"jsonrpc", "2.0"}, {"id", 42}};
    auto response = server_->ProcessRequest(MakeRawRequest(without_method));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("error"));
    EXPECT_EQ(-32600, json.at("error").at("code").get<int>());
    EXPECT_EQ(42, json.at("id").get<int>());
}

TEST_F(RpcServerUnitTest, UnknownMethodReturnsMethodNotFound)
{
    auto response = server_->ProcessRequest(MakeRequest("doesnotexist"));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("error"));
    EXPECT_EQ(-32601, json.at("error").at("code").get<int>());
    EXPECT_EQ("Method not found: doesnotexist", json.at("error").at("message").get<std::string>());
}

TEST_F(RpcServerUnitTest, RegisteredMethodIsInvoked)
{
    server_->RegisterMethod(
        "echo",
        [](const io::JsonValue& params)
        {
            nlohmann::json payload;
            payload["count"] = params.IsArray() ? params.Size() : 0;
            return io::JsonValue(std::move(payload));
        });

    nlohmann::json params = nlohmann::json::array({"a", "b", "c"});
    auto response = server_->ProcessRequest(MakeRequest("echo", params, 7));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("result"));
    EXPECT_EQ(3, json.at("result").at("count").get<int>());
    EXPECT_EQ(7, json.at("id").get<int>());
}

TEST_F(RpcServerUnitTest, DisabledMethodReturnsMethodNotFound)
{
    RpcConfig cfg;
    RpcServer disabled_server(cfg, std::shared_ptr<node::NeoSystem>{});
    disabled_server.AddDisabledMethod("getversion");

    auto response = disabled_server.ProcessRequest(MakeRequest("getversion"));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("error"));
    EXPECT_EQ(-32601, json.at("error").at("code").get<int>());
}

TEST_F(RpcServerUnitTest, SessionEndpointsReturnSessionsDisabledWhenDisabled)
{
    RpcConfig cfg;
    cfg.enable_sessions = false;
    RpcServer server(cfg, std::shared_ptr<node::NeoSystem>{});

    auto create_response = server.ProcessRequest(MakeRequest("createsession"));
    ASSERT_TRUE(create_response.GetJson().contains("error"));
    EXPECT_EQ(static_cast<int>(ErrorCode::SessionsDisabled),
              create_response.GetJson().at("error").at("code").get<int>());

    nlohmann::json params = nlohmann::json::array({"session", "iter"});
    auto traverse_response = server.ProcessRequest(MakeRequest("traverseiterator", params));
    ASSERT_TRUE(traverse_response.GetJson().contains("error"));
    EXPECT_EQ(static_cast<int>(ErrorCode::SessionsDisabled),
              traverse_response.GetJson().at("error").at("code").get<int>());

    auto terminate_response = server.ProcessRequest(MakeRequest("terminatesession", nlohmann::json::array({"session"})));
    ASSERT_TRUE(terminate_response.GetJson().contains("error"));
    EXPECT_EQ(static_cast<int>(ErrorCode::SessionsDisabled),
              terminate_response.GetJson().at("error").at("code").get<int>());
}

TEST_F(RpcServerUnitTest, RemovingDisabledMethodRestoresAccess)
{
    RpcServer server(RpcConfig{}, std::shared_ptr<node::NeoSystem>{});
    server.AddDisabledMethod("getversion");
    server.RemoveDisabledMethod("getversion");

    auto response = server.ProcessRequest(MakeRequest("getversion"));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("result"));
}

#ifdef NEO_HAS_HTTPLIB
class RpcServerAuthAdapter : public RpcServer
{
  public:
    using RpcServer::RpcServer;
    bool AuthenticateHeader(const std::string& header) const
    {
        httplib::Request req;
        if (!header.empty()) req.set_header("Authorization", header);
        return IsAuthenticated(req);
    }
};

TEST(RpcServerAuthTest, BasicAuthenticationValidatesCredentials)
{
    RpcConfig cfg;
    RpcServerAuthAdapter server(cfg, std::shared_ptr<node::NeoSystem>{});
    server.SetBasicAuth("admin", "secret");

    const std::string token = std::string("admin:secret");
    const std::string header = std::string("Basic ") + cryptography::Base64::Encode(token);

    EXPECT_TRUE(server.AuthenticateHeader(header));
    EXPECT_FALSE(server.AuthenticateHeader("Basic badtoken"));
    EXPECT_FALSE(server.AuthenticateHeader(""));

    server.DisableAuthentication();
    EXPECT_TRUE(server.AuthenticateHeader(""));
}
#endif

TEST_F(RpcServerUnitTest, PluginRequestHandlerProvidesFallback)
{
    server_->RegisterRequestHandler(
        [](const std::string& method, const io::JsonValue& params)
        {
            nlohmann::json payload;
            payload["method"] = method;
            payload["paramCount"] = params.IsArray() ? params.Size() : 0;
            return io::JsonValue(std::move(payload));
        });

    auto response = server_->ProcessRequest(MakeRequest("custommethod", nlohmann::json::array({1, 2}))); 
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("result"));
    EXPECT_EQ("custommethod", json.at("result").at("method").get<std::string>());
    EXPECT_EQ(2, json.at("result").at("paramCount").get<int>());
}

TEST_F(RpcServerUnitTest, GetVersionReturnsDefaultsWithoutNeoSystem)
{
    auto response = server_->ProcessRequest(MakeRequest("getversion"));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("result"));
    const auto& version = json.at("result");
    EXPECT_TRUE(version.contains("tcpport"));
    EXPECT_TRUE(version.contains("nonce"));
    EXPECT_TRUE(version.contains("useragent"));
    EXPECT_TRUE(version.contains("rpc"));
}

TEST_F(RpcServerUnitTest, GetPeersReturnsEmptyListsWhenNetworkingUnavailable)
{
    auto response = server_->ProcessRequest(MakeRequest(" getpeers "));
    const auto& json = response.GetJson();

    ASSERT_TRUE(json.contains("result"));
    const auto& peers = json.at("result");
    EXPECT_TRUE(peers.at("connected").empty());
    EXPECT_TRUE(peers.at("unconnected").empty());
    EXPECT_TRUE(peers.at("bad").empty());
}
}  // namespace neo::rpc::tests
