/**
 * @file tcp_client.h
 * @brief Tcp Client
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/network/ip_endpoint.h>
#include <neo/network/tcp_connection.h>

#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace neo::network
{
/**
 * @brief Represents a TCP client.
 */
class TcpClient
{
   public:
    /**
     * @brief Constructs a TcpClient.
     */
    TcpClient();

    /**
     * @brief Constructs a TcpClient with an existing io_context.
     * @param ioContext The IO context to use.
     */
    explicit TcpClient(boost::asio::io_context& ioContext);

    /**
     * @brief Destructor.
     */
    ~TcpClient();

    /**
     * @brief Connects to a server.
     * @param endpoint The endpoint to connect to.
     * @param timeout The timeout in milliseconds (0 for no timeout).
     * @return The connection.
     */
    std::shared_ptr<TcpConnection> Connect(const IPEndPoint& endpoint, uint32_t timeout = 10000);

    /**
     * @brief Connects to a server asynchronously.
     * @param endpoint The endpoint to connect to.
     * @param callback The callback to invoke when the connection completes.
     */
    void ConnectAsync(const IPEndPoint& endpoint,
                      std::function<void(std::shared_ptr<TcpConnection>, const std::error_code&)> callback);

    /**
     * @brief Stops the client.
     */
    void Stop();

   private:
    std::unique_ptr<boost::asio::io_context> ioContext_;
    boost::asio::io_context* ioContextPtr_;  // Points to internal or external io_context
    std::vector<std::thread> ioThreads_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> workGuard_;
    std::atomic<bool> running_;
    std::mutex connectionsMutex_;
    std::unordered_map<std::string, std::shared_ptr<TcpConnection>> connections_;

    void HandleConnectionClosed(const std::string& endpoint);
};
}  // namespace neo::network
