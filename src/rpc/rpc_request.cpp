/**
 * @file rpc_request.cpp
 * @brief Rpc Request
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/rpc/rpc_request.h>

namespace neo::rpc
{
RpcRequest::RpcRequest() : jsonrpc_("2.0") {}

RpcRequest::RpcRequest(const std::string& jsonrpc, const std::string& method, const nlohmann::json& params,
                       const nlohmann::json& id)
    : jsonrpc_(jsonrpc), method_(method), params_(params), id_(id)
{
}

const std::string& RpcRequest::GetJsonRpc() const { return jsonrpc_; }

void RpcRequest::SetJsonRpc(const std::string& jsonrpc) { jsonrpc_ = jsonrpc; }

const std::string& RpcRequest::GetMethod() const { return method_; }

void RpcRequest::SetMethod(const std::string& method) { method_ = method; }

const nlohmann::json& RpcRequest::GetParams() const { return params_; }

void RpcRequest::SetParams(const nlohmann::json& params) { params_ = params; }

const nlohmann::json& RpcRequest::GetId() const { return id_; }

void RpcRequest::SetId(const nlohmann::json& id) { id_ = id; }

nlohmann::json RpcRequest::ToJson() const
{
    nlohmann::json json;
    json["jsonrpc"] = jsonrpc_;
    json["method"] = method_;
    json["params"] = params_;
    json["id"] = id_;
    return json;
}

RpcRequest RpcRequest::FromJson(const nlohmann::json& json)
{
    RpcRequest request;

    if (json.contains("jsonrpc") && json["jsonrpc"].is_string()) request.SetJsonRpc(json["jsonrpc"].get<std::string>());

    if (json.contains("method") && json["method"].is_string()) request.SetMethod(json["method"].get<std::string>());

    if (json.contains("params")) request.SetParams(json["params"]);

    if (json.contains("id")) request.SetId(json["id"]);

    return request;
}
}  // namespace neo::rpc
