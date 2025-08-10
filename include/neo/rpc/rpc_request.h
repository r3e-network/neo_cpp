#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace neo::rpc
{
/**
 * @brief Represents an RPC request.
 */
class RpcRequest
{
   public:
    /**
     * @brief Constructs an RpcRequest.
     */
    RpcRequest();

    /**
     * @brief Constructs an RpcRequest with the specified values.
     * @param jsonrpc The JSON-RPC version.
     * @param method The method name.
     * @param params The parameters.
     * @param id The request ID.
     */
    RpcRequest(const std::string& jsonrpc, const std::string& method, const nlohmann::json& params,
               const nlohmann::json& id);

    /**
     * @brief Gets the JSON-RPC version.
     * @return The JSON-RPC version.
     */
    const std::string& GetJsonRpc() const;

    /**
     * @brief Sets the JSON-RPC version.
     * @param jsonrpc The JSON-RPC version.
     */
    void SetJsonRpc(const std::string& jsonrpc);

    /**
     * @brief Gets the method name.
     * @return The method name.
     */
    const std::string& GetMethod() const;

    /**
     * @brief Sets the method name.
     * @param method The method name.
     */
    void SetMethod(const std::string& method);

    /**
     * @brief Gets the parameters.
     * @return The parameters.
     */
    const nlohmann::json& GetParams() const;

    /**
     * @brief Sets the parameters.
     * @param params The parameters.
     */
    void SetParams(const nlohmann::json& params);

    /**
     * @brief Gets the request ID.
     * @return The request ID.
     */
    const nlohmann::json& GetId() const;

    /**
     * @brief Sets the request ID.
     * @param id The request ID.
     */
    void SetId(const nlohmann::json& id);

    /**
     * @brief Converts the request to JSON.
     * @return The JSON representation of the request.
     */
    nlohmann::json ToJson() const;

    /**
     * @brief Parses a request from JSON.
     * @param json The JSON to parse.
     * @return The parsed request.
     */
    static RpcRequest FromJson(const nlohmann::json& json);

   private:
    std::string jsonrpc_;
    std::string method_;
    nlohmann::json params_;
    nlohmann::json id_;
};
}  // namespace neo::rpc
