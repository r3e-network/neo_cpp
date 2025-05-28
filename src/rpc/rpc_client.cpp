#include <neo/rpc/rpc_client.h>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>

namespace neo::rpc
{
    // SimpleHttpClient implementation
    SimpleHttpClient::SimpleHttpClient()
    {
    }

    void SimpleHttpClient::SetBasicAuth(const std::string& username, const std::string& password)
    {
        // Encode credentials using Base64 as per HTTP Basic Authentication standard
        std::string credentials = username + ":" + password;
        std::string encoded_credentials = cryptography::Base64::Encode(
            io::ByteSpan(reinterpret_cast<const uint8_t*>(credentials.data()), credentials.size())
        );
        auth_header_ = "Basic " + encoded_credentials;
    }

    std::string SimpleHttpClient::Post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers)
    {
        // Simplified HTTP client implementation
        // In production, this should use a proper HTTP library like libcurl or cpp-httplib

        // For testing purposes, return a mock response
        std::string mock_response = R"({
            "jsonrpc": "2.0",
            "id": 1,
            "result": "mock_result"
        })";

        // Simulate network delay
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        return mock_response;
    }

    std::future<std::string> SimpleHttpClient::PostAsync(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers)
    {
        return std::async(std::launch::async, [this, url, content, headers]() {
            return Post(url, content, headers);
        });
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

    RpcClient::RpcClient(const std::string& base_url, const std::string& username, const std::string& password, std::unique_ptr<IHttpClient> http_client)
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
        return std::async(std::launch::async, [this, request, throw_on_error]() {
            return Send(request, throw_on_error);
        });
    }

    json::JToken RpcClient::RpcSend(const std::string& method, const std::vector<json::JToken>& params)
    {
        auto request = CreateRequest(method, params);
        auto response = Send(request);
        return response.GetResult();
    }

    std::future<json::JToken> RpcClient::RpcSendAsync(const std::string& method, const std::vector<json::JToken>& params)
    {
        return std::async(std::launch::async, [this, method, params]() {
            return RpcSend(method, params);
        });
    }

    // Blockchain methods
    std::string RpcClient::GetBestBlockHash()
    {
        auto result = RpcSend("getbestblockhash");
        return result.AsString();
    }

    std::future<std::string> RpcClient::GetBestBlockHashAsync()
    {
        return std::async(std::launch::async, [this]() {
            return GetBestBlockHash();
        });
    }

    uint32_t RpcClient::GetBlockCount()
    {
        auto result = RpcSend("getblockcount");
        return static_cast<uint32_t>(result.AsNumber());
    }

    std::future<uint32_t> RpcClient::GetBlockCountAsync()
    {
        return std::async(std::launch::async, [this]() {
            return GetBlockCount();
        });
    }

    json::JToken RpcClient::GetBlock(const std::string& hash, bool verbose)
    {
        std::vector<json::JToken> params;
        params.push_back(json::JString(hash));
        params.push_back(json::JBoolean(verbose));
        return RpcSend("getblock", params);
    }

    std::future<json::JToken> RpcClient::GetBlockAsync(const std::string& hash, bool verbose)
    {
        return std::async(std::launch::async, [this, hash, verbose]() {
            return GetBlock(hash, verbose);
        });
    }

    json::JToken RpcClient::GetBlock(uint32_t index, bool verbose)
    {
        std::vector<json::JToken> params;
        params.push_back(json::JNumber(static_cast<double>(index)));
        params.push_back(json::JBoolean(verbose));
        return RpcSend("getblock", params);
    }

    std::future<json::JToken> RpcClient::GetBlockAsync(uint32_t index, bool verbose)
    {
        return std::async(std::launch::async, [this, index, verbose]() {
            return GetBlock(index, verbose);
        });
    }

    json::JToken RpcClient::GetTransaction(const std::string& hash, bool verbose)
    {
        std::vector<json::JToken> params;
        params.push_back(json::JString(hash));
        params.push_back(json::JBoolean(verbose));
        return RpcSend("getrawtransaction", params);
    }

    std::future<json::JToken> RpcClient::GetTransactionAsync(const std::string& hash, bool verbose)
    {
        return std::async(std::launch::async, [this, hash, verbose]() {
            return GetTransaction(hash, verbose);
        });
    }

    std::string RpcClient::SendRawTransaction(const std::string& hex)
    {
        std::vector<json::JToken> params;
        params.push_back(json::JString(hex));
        auto result = RpcSend("sendrawtransaction", params);
        return result.AsString();
    }

    std::future<std::string> RpcClient::SendRawTransactionAsync(const std::string& hex)
    {
        return std::async(std::launch::async, [this, hex]() {
            return SendRawTransaction(hex);
        });
    }

    json::JToken RpcClient::InvokeFunction(const std::string& script_hash, const std::string& operation, const std::vector<json::JToken>& params)
    {
        std::vector<json::JToken> rpc_params;
        rpc_params.push_back(json::JString(script_hash));
        rpc_params.push_back(json::JString(operation));

        if (!params.empty())
        {
            auto params_array = json::JArray();
            for (const auto& param : params)
            {
                params_array.Add(param);
            }
            rpc_params.push_back(params_array);
        }

        return RpcSend("invokefunction", rpc_params);
    }

    std::future<json::JToken> RpcClient::InvokeFunctionAsync(const std::string& script_hash, const std::string& operation, const std::vector<json::JToken>& params)
    {
        return std::async(std::launch::async, [this, script_hash, operation, params]() {
            return InvokeFunction(script_hash, operation, params);
        });
    }

    json::JToken RpcClient::GetVersion()
    {
        return RpcSend("getversion");
    }

    std::future<json::JToken> RpcClient::GetVersionAsync()
    {
        return std::async(std::launch::async, [this]() {
            return GetVersion();
        });
    }

    RpcRequest RpcClient::CreateRequest(const std::string& method, const std::vector<json::JToken>& params)
    {
        nlohmann::json json_params = nlohmann::json::array();
        for (const auto& param : params)
        {
            // Convert JToken to nlohmann::json
            // This is a simplified conversion
            json_params.push_back(param.ToString());
        }

        return RpcRequest("2.0", method, json_params, nlohmann::json(next_id_++));
    }

    RpcResponse RpcClient::ProcessResponse(const std::string& response, bool throw_on_error)
    {
        try
        {
            auto json = nlohmann::json::parse(response);
            auto rpc_response = RpcResponse::FromJson(json);

            if (throw_on_error && rpc_response.GetError().GetCode() != 0)
            {
                throw std::runtime_error("RPC Error: " + rpc_response.GetError().GetMessage());
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
            RpcResponseError error;
            error.SetCode(-1);
            error.SetMessage("Failed to parse response");
            error_response.SetError(error);
            return error_response;
        }
    }
}
