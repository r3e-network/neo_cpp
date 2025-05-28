#pragma once

#include <neo/network/ip_endpoint.h>
#include <neo/network/tcp_connection.h>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <unordered_map>

namespace neo::network
{
    /**
     * @brief Represents a TCP server.
     */
    class TcpServer
    {
    public:
        /**
         * @brief Constructs a TcpServer.
         * @param endpoint The endpoint to listen on.
         * @param maxConnections The maximum number of connections allowed.
         */
        explicit TcpServer(const IPEndPoint& endpoint, uint32_t maxConnections = 1000);

        /**
         * @brief Destructor.
         */
        ~TcpServer();

        /**
         * @brief Starts the server.
         */
        void Start();

        /**
         * @brief Stops the server.
         */
        void Stop();

        /**
         * @brief Gets the endpoint.
         * @return The endpoint.
         */
        const IPEndPoint& GetEndpoint() const;

        /**
         * @brief Gets the number of active connections.
         * @return The number of active connections.
         */
        uint32_t GetConnectionCount() const;

        /**
         * @brief Sets the connection accepted callback.
         * @param callback The callback.
         */
        void SetConnectionAcceptedCallback(std::function<void(std::shared_ptr<TcpConnection>)> callback);

    private:
        IPEndPoint endpoint_;
        std::unique_ptr<boost::asio::io_context> ioContext_;
        std::unique_ptr<boost::asio::basic_socket_acceptor<boost::asio::ip::tcp>> acceptor_;
        std::vector<std::thread> ioThreads_;
        std::unique_ptr<boost::asio::executor_work_guard<
            boost::asio::io_context::executor_type>> workGuard_;
        std::atomic<bool> running_;
        std::function<void(std::shared_ptr<TcpConnection>)> connectionAcceptedCallback_;
        std::unordered_map<std::string, std::shared_ptr<TcpConnection>> connections_;
        mutable std::mutex connectionsMutex_;
        uint32_t maxConnections_;

        void AcceptConnection();
        void HandleAccept(std::shared_ptr<TcpConnection> connection, const std::error_code& error);
        void RemoveConnection(const std::string& endpoint);
    };
}
