#pragma once

#include <neo/node/neo_system.h>
#include <neo/io/json.h>
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace neo::rpc
{
    /**
     * @brief Represents an RPC server.
     */
    class RPCServer
    {
    public:
        /**
         * @brief Constructs an RPCServer.
         * @param neoSystem The Neo system.
         * @param port The port.
         * @param enableCors Whether to enable CORS.
         * @param enableAuth Whether to enable authentication.
         * @param username The username.
         * @param password The password.
         */
        RPCServer(std::shared_ptr<node::NeoSystem> neoSystem, uint16_t port, bool enableCors = false, bool enableAuth = false, const std::string& username = "", const std::string& password = "");

        /**
         * @brief Destructor.
         */
        ~RPCServer();

        /**
         * @brief Starts the server.
         */
        void Start();

        /**
         * @brief Stops the server.
         */
        void Stop();

        /**
         * @brief Checks if the server is running.
         * @return True if the server is running, false otherwise.
         */
        bool IsRunning() const;

        /**
         * @brief Gets the port.
         * @return The port.
         */
        uint16_t GetPort() const;

        /**
         * @brief Gets the Neo system.
         * @return The Neo system.
         */
        std::shared_ptr<node::NeoSystem> GetNeoSystem() const;

        /**
         * @brief Registers an RPC method.
         * @param method The method name.
         * @param handler The handler.
         */
        void RegisterMethod(const std::string& method, std::function<nlohmann::json(const nlohmann::json&)> handler);

        /**
         * @brief Unregisters an RPC method.
         * @param method The method name.
         */
        void UnregisterMethod(const std::string& method);

    private:
        std::shared_ptr<node::NeoSystem> neoSystem_;
        uint16_t port_;
        bool enableCors_;
        bool enableAuth_;
        std::string username_;
        std::string password_;
        std::atomic<bool> running_;
        std::thread serverThread_;
        std::mutex mutex_;
        std::condition_variable condition_;
        std::unordered_map<std::string, std::function<nlohmann::json(const nlohmann::json&)>> methods_;

        /**
         * @brief Runs the server.
         */
        void RunServer();

        /**
         * @brief Handles an HTTP request.
         * @param request The request.
         * @return The response.
         */
        std::string HandleRequest(const std::string& request);

        /**
         * @brief Handles an RPC request.
         * @param request The request.
         * @return The response.
         */
        nlohmann::json HandleRPCRequest(const nlohmann::json& request);

        /**
         * @brief Creates an error response.
         * @param id The request ID.
         * @param code The error code.
         * @param message The error message.
         * @return The error response.
         */
        nlohmann::json CreateErrorResponse(const nlohmann::json& id, int32_t code, const std::string& message);

        /**
         * @brief Creates a success response.
         * @param id The request ID.
         * @param result The result.
         * @return The success response.
         */
        nlohmann::json CreateSuccessResponse(const nlohmann::json& id, const nlohmann::json& result);

        /**
         * @brief Initializes the RPC methods.
         */
        void InitializeMethods();
    };
}
