#pragma once

#include <neo/rpc/rpc_request.h>
#include <neo/rpc/rpc_response.h>
#include <neo/json/json.h>
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <future>
#include <functional>
#include <map>

namespace neo::rpc
{
    /**
     * @brief HTTP client interface for making HTTP requests.
     */
    class IHttpClient
    {
    public:
        virtual ~IHttpClient() = default;

        /**
         * @brief Makes a synchronous HTTP POST request.
         * @param url The URL to send the request to.
         * @param content The request content.
         * @param headers Additional headers.
         * @return The response content.
         */
        virtual std::string Post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers = {}) = 0;

        /**
         * @brief Makes an asynchronous HTTP POST request.
         * @param url The URL to send the request to.
         * @param content The request content.
         * @param headers Additional headers.
         * @return A future containing the response content.
         */
        virtual std::future<std::string> PostAsync(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers = {}) = 0;
    };

    /**
     * @brief Simple HTTP client implementation.
     */
    class SimpleHttpClient : public IHttpClient
    {
    public:
        /**
         * @brief Constructor.
         */
        SimpleHttpClient();

        /**
         * @brief Destructor.
         */
        ~SimpleHttpClient() override = default;

        /**
         * @brief Sets basic authentication credentials.
         * @param username The username.
         * @param password The password.
         */
        void SetBasicAuth(const std::string& username, const std::string& password);

        // IHttpClient implementation
        std::string Post(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers = {}) override;
        std::future<std::string> PostAsync(const std::string& url, const std::string& content, const std::map<std::string, std::string>& headers = {}) override;

    private:
        std::string auth_header_;
    };

    /**
     * @brief RPC client for making calls to Neo nodes.
     */
    class RpcClient
    {
    public:
        /**
         * @brief Constructor.
         * @param base_url The base URL of the Neo node.
         * @param http_client Optional HTTP client implementation.
         */
        explicit RpcClient(const std::string& base_url, std::unique_ptr<IHttpClient> http_client = nullptr);

        /**
         * @brief Constructor with authentication.
         * @param base_url The base URL of the Neo node.
         * @param username The username for basic authentication.
         * @param password The password for basic authentication.
         * @param http_client Optional HTTP client implementation.
         */
        RpcClient(const std::string& base_url, const std::string& username, const std::string& password, std::unique_ptr<IHttpClient> http_client = nullptr);

        /**
         * @brief Destructor.
         */
        ~RpcClient() = default;

        /**
         * @brief Sends an RPC request synchronously.
         * @param request The RPC request.
         * @param throw_on_error Whether to throw an exception on error.
         * @return The RPC response.
         */
        RpcResponse Send(const RpcRequest& request, bool throw_on_error = true);

        /**
         * @brief Sends an RPC request asynchronously.
         * @param request The RPC request.
         * @param throw_on_error Whether to throw an exception on error.
         * @return A future containing the RPC response.
         */
        std::future<RpcResponse> SendAsync(const RpcRequest& request, bool throw_on_error = true);

        /**
         * @brief Sends an RPC request with method and parameters.
         * @param method The RPC method name.
         * @param params The parameters.
         * @return The result JSON.
         */
        nlohmann::json RpcSend(const std::string& method, const std::vector<nlohmann::json>& params = {});

        /**
         * @brief Sends an RPC request with method and parameters asynchronously.
         * @param method The RPC method name.
         * @param params The parameters.
         * @return A future containing the result JSON.
         */
        std::future<nlohmann::json> RpcSendAsync(const std::string& method, const std::vector<nlohmann::json>& params = {});

        // Blockchain methods
        /**
         * @brief Gets the best block hash.
         * @return The best block hash.
         */
        std::string GetBestBlockHash();

        /**
         * @brief Gets the best block hash asynchronously.
         * @return A future containing the best block hash.
         */
        std::future<std::string> GetBestBlockHashAsync();

        /**
         * @brief Gets the block count.
         * @return The block count.
         */
        uint32_t GetBlockCount();

        /**
         * @brief Gets the block count asynchronously.
         * @return A future containing the block count.
         */
        std::future<uint32_t> GetBlockCountAsync();

        /**
         * @brief Gets a block by hash.
         * @param hash The block hash.
         * @param verbose Whether to return verbose information.
         * @return The block information.
         */
        nlohmann::json GetBlock(const std::string& hash, bool verbose = true);

        /**
         * @brief Gets a block by hash asynchronously.
         * @param hash The block hash.
         * @param verbose Whether to return verbose information.
         * @return A future containing the block information.
         */
        std::future<nlohmann::json> GetBlockAsync(const std::string& hash, bool verbose = true);

        /**
         * @brief Gets a block by index.
         * @param index The block index.
         * @param verbose Whether to return verbose information.
         * @return The block information.
         */
        nlohmann::json GetBlock(uint32_t index, bool verbose = true);

        /**
         * @brief Gets a block by index asynchronously.
         * @param index The block index.
         * @param verbose Whether to return verbose information.
         * @return A future containing the block information.
         */
        std::future<nlohmann::json> GetBlockAsync(uint32_t index, bool verbose = true);

        /**
         * @brief Gets a transaction by hash.
         * @param hash The transaction hash.
         * @param verbose Whether to return verbose information.
         * @return The transaction information.
         */
        nlohmann::json GetTransaction(const std::string& hash, bool verbose = true);

        /**
         * @brief Gets a transaction by hash asynchronously.
         * @param hash The transaction hash.
         * @param verbose Whether to return verbose information.
         * @return A future containing the transaction information.
         */
        std::future<nlohmann::json> GetTransactionAsync(const std::string& hash, bool verbose = true);

        /**
         * @brief Sends a raw transaction.
         * @param hex The transaction hex string.
         * @return The transaction hash.
         */
        std::string SendRawTransaction(const std::string& hex);

        /**
         * @brief Sends a raw transaction asynchronously.
         * @param hex The transaction hex string.
         * @return A future containing the transaction hash.
         */
        std::future<std::string> SendRawTransactionAsync(const std::string& hex);

        /**
         * @brief Invokes a smart contract function.
         * @param script_hash The contract script hash.
         * @param operation The operation name.
         * @param params The parameters.
         * @return The invocation result.
         */
        nlohmann::json InvokeFunction(const std::string& script_hash, const std::string& operation, const std::vector<nlohmann::json>& params = {});

        /**
         * @brief Invokes a smart contract function asynchronously.
         * @param script_hash The contract script hash.
         * @param operation The operation name.
         * @param params The parameters.
         * @return A future containing the invocation result.
         */
        std::future<nlohmann::json> InvokeFunctionAsync(const std::string& script_hash, const std::string& operation, const std::vector<nlohmann::json>& params = {});

        /**
         * @brief Gets the version information.
         * @return The version information.
         */
        nlohmann::json GetVersion();

        /**
         * @brief Gets the version information asynchronously.
         * @return A future containing the version information.
         */
        std::future<nlohmann::json> GetVersionAsync();

    private:
        std::string base_url_;
        std::unique_ptr<IHttpClient> http_client_;
        uint64_t next_id_;

        /**
         * @brief Creates an RPC request.
         * @param method The method name.
         * @param params The parameters.
         * @return The RPC request.
         */
        RpcRequest CreateRequest(const std::string& method, const std::vector<nlohmann::json>& params = {});

        /**
         * @brief Processes an RPC response.
         * @param response The response content.
         * @param throw_on_error Whether to throw an exception on error.
         * @return The RPC response.
         */
        RpcResponse ProcessResponse(const std::string& response, bool throw_on_error);
    };
}
