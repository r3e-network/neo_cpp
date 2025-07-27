#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace neo::rpc
{
/**
 * @brief Represents an RPC response.
 */
class RpcResponse
{
  public:
    /**
     * @brief Constructs an RpcResponse.
     */
    RpcResponse();

    /**
     * @brief Constructs an RpcResponse with the specified values.
     * @param jsonrpc The JSON-RPC version.
     * @param result The result.
     * @param error The error.
     * @param id The request ID.
     */
    RpcResponse(const std::string& jsonrpc, const nlohmann::json& result, const nlohmann::json& error,
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
     * @brief Gets the result.
     * @return The result.
     */
    const nlohmann::json& GetResult() const;

    /**
     * @brief Sets the result.
     * @param result The result.
     */
    void SetResult(const nlohmann::json& result);

    /**
     * @brief Gets the error.
     * @return The error.
     */
    const nlohmann::json& GetError() const;

    /**
     * @brief Sets the error.
     * @param error The error.
     */
    void SetError(const nlohmann::json& error);

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
     * @brief Converts the response to JSON.
     * @return The JSON representation of the response.
     */
    nlohmann::json ToJson() const;

    /**
     * @brief Parses a response from JSON.
     * @param json The JSON to parse.
     * @return The parsed response.
     */
    static RpcResponse FromJson(const nlohmann::json& json);

    /**
     * @brief Creates a success response.
     * @param id The request ID.
     * @param result The result.
     * @return The success response.
     */
    static RpcResponse CreateSuccessResponse(const nlohmann::json& id, const nlohmann::json& result);

    /**
     * @brief Creates an error response.
     * @param id The request ID.
     * @param code The error code.
     * @param message The error message.
     * @param data The error data.
     * @return The error response.
     */
    static RpcResponse CreateErrorResponse(const nlohmann::json& id, int code, const std::string& message,
                                           const nlohmann::json& data = nullptr);

  private:
    std::string jsonrpc_;
    nlohmann::json result_;
    nlohmann::json error_;
    nlohmann::json id_;
};
}  // namespace neo::rpc
