#include <neo/rpc/rpc_response.h>

namespace neo::rpc
{
RpcResponse::RpcResponse() : jsonrpc_("2.0") {}

RpcResponse::RpcResponse(const std::string& jsonrpc, const nlohmann::json& result, const nlohmann::json& error,
                         const nlohmann::json& id)
    : jsonrpc_(jsonrpc), result_(result), error_(error), id_(id)
{
}

const std::string& RpcResponse::GetJsonRpc() const { return jsonrpc_; }

void RpcResponse::SetJsonRpc(const std::string& jsonrpc) { jsonrpc_ = jsonrpc; }

const nlohmann::json& RpcResponse::GetResult() const { return result_; }

void RpcResponse::SetResult(const nlohmann::json& result) { result_ = result; }

const nlohmann::json& RpcResponse::GetError() const { return error_; }

void RpcResponse::SetError(const nlohmann::json& error) { error_ = error; }

const nlohmann::json& RpcResponse::GetId() const { return id_; }

void RpcResponse::SetId(const nlohmann::json& id) { id_ = id; }

nlohmann::json RpcResponse::ToJson() const
{
    nlohmann::json json;
    json["jsonrpc"] = jsonrpc_;

    if (!result_.is_null()) json["result"] = result_;

    if (!error_.is_null()) json["error"] = error_;

    json["id"] = id_;
    return json;
}

RpcResponse RpcResponse::FromJson(const nlohmann::json& json)
{
    RpcResponse response;

    if (json.contains("jsonrpc") && json["jsonrpc"].is_string())
        response.SetJsonRpc(json["jsonrpc"].get<std::string>());

    if (json.contains("result")) response.SetResult(json["result"]);

    if (json.contains("error")) response.SetError(json["error"]);

    if (json.contains("id")) response.SetId(json["id"]);

    return response;
}

RpcResponse RpcResponse::CreateSuccessResponse(const nlohmann::json& id, const nlohmann::json& result)
{
    return RpcResponse("2.0", result, nullptr, id);
}

RpcResponse RpcResponse::CreateErrorResponse(const nlohmann::json& id, int code, const std::string& message,
                                             const nlohmann::json& data)
{
    nlohmann::json error;
    error["code"] = code;
    error["message"] = message;

    if (!data.is_null()) error["data"] = data;

    return RpcResponse("2.0", nullptr, error, id);
}
}  // namespace neo::rpc
