/**
 * @file tcp_connection.h
 * @brief TCP network connections
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/message.h>
#include <neo/network/thread_safe_queue.h>

#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>

namespace neo::network
{
/**
 * @brief Represents a TCP connection.
 */
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
   public:
    /**
     * @brief Constructs a TcpConnection.
     * @param ioContext The IO context.
     */
    explicit TcpConnection(boost::asio::io_context& ioContext);

    /**
     * @brief Destructor.
     */
    ~TcpConnection();

    /**
     * @brief Gets the socket.
     * @return The socket.
     */
    boost::asio::ip::tcp::socket& GetSocket();

    /**
     * @brief Gets the remote endpoint.
     * @return The remote endpoint.
     */
    IPEndPoint GetRemoteEndpoint() const;

    /**
     * @brief Gets a string representation of the remote endpoint.
     * @return A string representation of the remote endpoint.
     */
    std::string GetRemoteEndpointString() const;

    /**
     * @brief Starts the connection.
     */
    void Start();

    /**
     * @brief Stops the connection.
     */
    void Stop();

    /**
     * @brief Closes the connection.
     */
    void Close();

    /**
     * @brief Sends a message.
     * @param message The message.
     */
    void Send(const p2p::Message& message);

    /**
     * @brief Sets the message received callback.
     * @param callback The callback.
     */
    void SetMessageReceivedCallback(std::function<void(const p2p::Message&)> callback);

    /**
     * @brief Sets the connection closed callback.
     * @param callback The callback.
     */
    void SetConnectionClosedCallback(std::function<void()> callback);

   private:
    boost::asio::ip::tcp::socket socket_;
    std::atomic<bool> running_;
    std::function<void(const p2p::Message&)> messageReceivedCallback_;
    std::function<void()> connectionClosedCallback_;
    std::mutex sendMutex_;
    uint8_t receiveBuffer_[8192];  // Buffer for reading messages
    ThreadSafeQueue<std::vector<uint8_t>> sendQueue_;
    std::atomic<bool> sending_;

    void ReadMessage();
    void HandleReadHeader(const std::error_code& error, size_t bytesTransferred);
    void ReadPayload(p2p::MessageCommand command, uint32_t payloadLength, uint32_t checksum, p2p::MessageFlags flags);
    void HandleReadPayload(const std::error_code& error, size_t bytesTransferred, p2p::MessageCommand command,
                           uint32_t expectedLength, uint32_t checksum, p2p::MessageFlags flags);
    void ProcessSendQueue();
    void HandleWrite(const std::error_code& error, size_t bytesTransferred);
    void HandleError(const std::error_code& error);
    bool ValidateChecksum(const io::ByteVector& payload, uint32_t expectedChecksum);
};
}  // namespace neo::network
