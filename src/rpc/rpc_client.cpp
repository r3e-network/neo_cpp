/**
 * @file rpc_client.cpp
 * @brief Rpc Client
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <httplib.h>
#include <neo/core/safe_conversions.h>
#include <neo/cryptography/base64.h>
#include <neo/rpc/rpc_client.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace neo::rpc
{
namespace
{
std::string ParseStringResult(const nlohmann::json& result, const char* method)
{
    if (!result.is_string())
    {
        throw std::runtime_error(std::string("Unexpected RPC result type for ") + method);
    }
    return result.get<std::string>();
}

uint32_t ParseUInt32Result(const nlohmann::json& result, const char* method)
{
    if (result.is_number_unsigned())
    {
        return result.get<uint32_t>();
    }
    if (result.is_number_integer())
    {
        return static_cast<uint32_t>(result.get<int64_t>());
    }
    throw std::runtime_error(std::string("Unexpected RPC result type for ") + method);
}
}  // namespace

// SimpleHttpClient implementation
SimpleHttpClient::SimpleHttpClient() {}

void SimpleHttpClient::SetBasicAuth(const std::string& username, const std::string& password)
{
    // Encode credentials using Base64 as per HTTP Basic Authentication standard
    std::string credentials = username + ":" + password;
    std::string encoded_credentials = cryptography::Base64::Encode(
        io::ByteSpan(reinterpret_cast<const uint8_t*>(credentials.data()), credentials.size()));
    auth_header_ = "Basic " + encoded_credentials;
}

std::string SimpleHttpClient::Post(const std::string& url, const std::string& content,
                                   const std::map<std::string, std::string>& headers)
{
    // Parse URL to extract host, port, and path
    std::string protocol, host, path;
    int port = 80;

    size_t protocol_end = url.find("://");
    if (protocol_end != std::string::npos)
    {
        protocol = url.substr(0, protocol_end);
        size_t host_start = protocol_end + 3;
        size_t path_start = url.find('/', host_start);

        if (path_start != std::string::npos)
        {
            std::string host_port = url.substr(host_start, path_start - host_start);
            path = url.substr(path_start);

            size_t port_start = host_port.find(':');
            if (port_start != std::string::npos)
            {
                host = host_port.substr(0, port_start);
                try
                {
                    port = core::SafeConversions::SafeToPort(host_port.substr(port_start + 1));
                }
                catch (const std::exception& e)
                {
                    throw std::runtime_error("Invalid port in URL: " + std::string(e.what()));
                }
            }
            else
            {
                host = host_port;
                if (protocol == "https")
                {
                    port = 443;
                }
            }
        }
        else
        {
            std::string host_port = url.substr(host_start);
            path = "/";

            size_t port_start = host_port.find(':');
            if (port_start != std::string::npos)
            {
                host = host_port.substr(0, port_start);
                try
                {
                    port = core::SafeConversions::SafeToPort(host_port.substr(port_start + 1));
                }
                catch (const std::exception& e)
                {
                    throw std::runtime_error("Invalid port in URL: " + std::string(e.what()));
                }
            }
            else
            {
                host = host_port;
                if (protocol == "https")
                {
                    port = 443;
                }
            }
        }
    }
    else
    {
        throw std::runtime_error("Invalid URL: " + url);
    }

    // Create HTTP client
    std::unique_ptr<httplib::Client> cli;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    if (protocol == "https")
    {
        cli = std::make_unique<httplib::SSLClient>(host, port);
    }
    else
#endif
    {
        if (protocol == "https")
        {
            throw std::runtime_error("HTTPS support requires CPPHTTPLIB_OPENSSL_SUPPORT");
        }
        cli = std::make_unique<httplib::Client>(host, port);
    }

    // Set timeout
    cli->set_connection_timeout(30, 0);  // 30 seconds
    cli->set_read_timeout(30, 0);        // 30 seconds
    cli->set_write_timeout(30, 0);       // 30 seconds

    // Prepare headers
    httplib::Headers httplib_headers;
    for (const auto& [key, value] : headers)
    {
        httplib_headers.emplace(key, value);
    }

    // Add auth header if set
    if (!auth_header_.empty())
    {
        httplib_headers.emplace("Authorization", auth_header_);
    }

    // Make POST request
    auto res = cli->Post(path.c_str(), httplib_headers, content, "application/json");

    // Check response
    if (!res)
    {
        throw std::runtime_error("HTTP request failed: No response from server");
    }

    if (res->status != 200)
    {
        throw std::runtime_error("HTTP request failed with status " + std::to_string(res->status) + ": " + res->body);
    }

    return res->body;
}

std::future<std::string> SimpleHttpClient::PostAsync(const std::string& url, const std::string& content,
                                                     const std::map<std::string, std::string>& headers)
{
    return std::async(std::launch::async, [this, url, content, headers]() { return Post(url, content, headers); });
}

// RpcClient implementation
RpcClient::RpcClient(const std::string& base_url, std::unique_ptr<IHttpClient> http_client)
    : base_url_(base_url), next_id_(1)
{
    if (http_client)
    {
        http_client_ = std::move(http_client);
    }
    else
    {
        http_client_ = std::make_unique<SimpleHttpClient>();
    }
}

RpcClient::RpcClient(const std::string& base_url, const std::string& username, const std::string& password,
                     std::unique_ptr<IHttpClient> http_client)
    : base_url_(base_url), next_id_(1)
{
    if (http_client)
    {
        http_client_ = std::move(http_client);
    }
    else
    {
        auto simple_client = std::make_unique<SimpleHttpClient>();
        simple_client->SetBasicAuth(username, password);
        http_client_ = std::move(simple_client);
    }
}

RpcResponse RpcClient::Send(const RpcRequest& request, bool throw_on_error)
{
    auto request_json = request.ToJson();
    std::string request_content = request_json.dump();

    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";

    std::string response_content = http_client_->Post(base_url_, request_content, headers);

    return ProcessResponse(response_content, throw_on_error);
}

std::future<RpcResponse> RpcClient::SendAsync(const RpcRequest& request, bool throw_on_error)
{
    auto request_json = request.ToJson();
    std::string request_content = request_json.dump();

    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";

    auto response_future = http_client_->PostAsync(base_url_, request_content, headers);

    return std::async(std::launch::async,
                      [this, throw_on_error, response_future = std::move(response_future)]() mutable
                      {
                          auto response_content = response_future.get();
                          return ProcessResponse(response_content, throw_on_error);
                      });
}

nlohmann::json RpcClient::RpcSend(const std::string& method, const std::vector<nlohmann::json>& params)
{
    auto request = CreateRequest(method, params);
    auto response = Send(request);
    return response.GetResult();
}

std::future<nlohmann::json> RpcClient::RpcSendAsync(const std::string& method,
                                                   const std::vector<nlohmann::json>& params)
{
    auto request = CreateRequest(method, params);
    auto response_future = SendAsync(request);

    return std::async(std::launch::async, [response_future = std::move(response_future)]() mutable
                      {
                          auto response = response_future.get();
                          return response.GetResult();
                      });
}

// Blockchain methods
std::string RpcClient::GetBestBlockHash()
{
    auto result = RpcSend("getbestblockhash");
    return ParseStringResult(result, "getbestblockhash");
}

std::future<std::string> RpcClient::GetBestBlockHashAsync()
{
    auto future = RpcSendAsync("getbestblockhash");
    return std::async(std::launch::async, [future = std::move(future)]() mutable
                      { return ParseStringResult(future.get(), "getbestblockhash"); });
}

uint32_t RpcClient::GetBlockCount()
{
    auto result = RpcSend("getblockcount");
    return ParseUInt32Result(result, "getblockcount");
}

std::future<uint32_t> RpcClient::GetBlockCountAsync()
{
    auto future = RpcSendAsync("getblockcount");
    return std::async(std::launch::async, [future = std::move(future)]() mutable
                      { return ParseUInt32Result(future.get(), "getblockcount"); });
}

nlohmann::json RpcClient::GetBlock(const std::string& hash, bool verbose)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(hash);
    params.emplace_back(verbose);
    return RpcSend("getblock", params);
}

std::future<nlohmann::json> RpcClient::GetBlockAsync(const std::string& hash, bool verbose)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(hash);
    params.emplace_back(verbose);
    return RpcSendAsync("getblock", params);
}

nlohmann::json RpcClient::GetBlock(uint32_t index, bool verbose)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(index);
    params.emplace_back(verbose);
    return RpcSend("getblock", params);
}

std::future<nlohmann::json> RpcClient::GetBlockAsync(uint32_t index, bool verbose)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(index);
    params.emplace_back(verbose);
    return RpcSendAsync("getblock", params);
}

nlohmann::json RpcClient::GetTransaction(const std::string& hash, bool verbose)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(hash);
    params.emplace_back(verbose);
    return RpcSend("getrawtransaction", params);
}

std::future<nlohmann::json> RpcClient::GetTransactionAsync(const std::string& hash, bool verbose)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(hash);
    params.emplace_back(verbose);
    return RpcSendAsync("getrawtransaction", params);
}

std::string RpcClient::SendRawTransaction(const std::string& hex)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(hex);
    auto result = RpcSend("sendrawtransaction", params);
    if (result.is_string())
    {
        return result.get<std::string>();
    }
    return result.dump();
}

std::future<std::string> RpcClient::SendRawTransactionAsync(const std::string& hex)
{
    std::vector<nlohmann::json> params;
    params.emplace_back(hex);
    auto future = RpcSendAsync("sendrawtransaction", params);
    return std::async(std::launch::async, [future = std::move(future)]() mutable
                      {
                          auto result = future.get();
                          if (result.is_string())
                          {
                              return result.get<std::string>();
                          }
                          return result.dump();
                      });
}

nlohmann::json RpcClient::InvokeFunction(const std::string& script_hash, const std::string& operation,
                                        const std::vector<nlohmann::json>& params)
{
    std::vector<nlohmann::json> rpc_params;
    rpc_params.emplace_back(script_hash);
    rpc_params.emplace_back(operation);

    if (!params.empty())
    {
        rpc_params.emplace_back(params);
    }

    return RpcSend("invokefunction", rpc_params);
}

std::future<nlohmann::json> RpcClient::InvokeFunctionAsync(const std::string& script_hash,
                                                           const std::string& operation,
                                                           const std::vector<nlohmann::json>& params)
{
    std::vector<nlohmann::json> rpc_params;
    rpc_params.emplace_back(script_hash);
    rpc_params.emplace_back(operation);

    if (!params.empty())
    {
        rpc_params.emplace_back(params);
    }

    return RpcSendAsync("invokefunction", rpc_params);
}

nlohmann::json RpcClient::GetVersion() { return RpcSend("getversion"); }

std::future<nlohmann::json> RpcClient::GetVersionAsync()
{
    return RpcSendAsync("getversion");
}

RpcRequest RpcClient::CreateRequest(const std::string& method, const std::vector<nlohmann::json>& params)
{
    nlohmann::json json_params = nlohmann::json::array();
    json_params = params;

    return RpcRequest("2.0", method, json_params, nlohmann::json(next_id_++));
}

RpcResponse RpcClient::ProcessResponse(const std::string& response, bool throw_on_error)
{
    try
    {
        auto json = nlohmann::json::parse(response);
        auto rpc_response = RpcResponse::FromJson(json);

        const auto& errorJson = rpc_response.GetError();
        if (throw_on_error && !errorJson.is_null())
        {
            const int code = errorJson.value("code", -1);
            const std::string message = errorJson.value("message", std::string("RPC error"));
            throw std::runtime_error("RPC Error: " + message + " (code " + std::to_string(code) + ")");
        }

        return rpc_response;
    }
    catch (const std::exception& e)
    {
        if (throw_on_error)
        {
            throw std::runtime_error("Failed to parse RPC response: " + std::string(e.what()));
        }

        // Return error response
        RpcResponse error_response;
        nlohmann::json errorJson;
        errorJson["code"] = -1;
        errorJson["message"] = "Failed to parse response";
        error_response.SetError(errorJson);
        error_response.SetResult(nullptr);
        return error_response;
    }
}
}  // namespace neo::rpc
