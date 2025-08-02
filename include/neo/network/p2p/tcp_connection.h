#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <neo/io/byte_vector.h>
#include <neo/network/ip_endpoint.h>
#include <neo/network/p2p/connection.h>
#include <thread>

namespace neo::network::p2p
{
// Forward declaration
class Message;

/**
 * @brief Represents a TCP connection to a remote node.
 */
class TcpConnection : public Connection, public std::enable_shared_from_this<TcpConnection>
{
  public:
    /**
     * @brief Creates a TcpConnection.
     * @param socket The socket.
     * @return The TcpConnection.
     */
    static std::shared_ptr<TcpConnection> Create(boost::asio::ip::tcp::socket socket);

    /**
     * @brief Constructs a TcpConnection.
     * @param socket The socket.
     */
    explicit TcpConnection(boost::asio::ip::tcp::socket socket);

    /**
     * @brief Destructor.
     */
    ~TcpConnection() override;

    /**
     * @brief Gets the remote endpoint.
     * @return The remote endpoint.
     */
    IPEndPoint GetRemoteEndPoint() const override;

    /**
     * @brief Gets the local endpoint.
     * @return The local endpoint.
     */
    IPEndPoint GetLocalEndPoint() const override;

    /**
     * @brief Sends a message to the remote node.
     * @param message The message to send.
     * @param enableCompression Whether to enable compression.
     * @return True if the message was sent successfully, false otherwise.
     */
    bool Send(const Message& message, bool enableCompression = true) override;

    /**
     * @brief Disconnects from the remote node.
     */
    void Disconnect() override;

    /**
     * @brief Starts receiving messages.
     */
    void StartReceiving();

  private:
    boost::asio::ip::tcp::socket socket_;
    std::atomic<bool> connected_;
    std::mutex sendMutex_;
    io::ByteVector receiveBuffer_;

    void DoReceive();
    void HandleReceive(const std::error_code& error, std::size_t bytesTransferred);
    void HandleSend(const std::error_code& error, std::size_t bytesTransferred);
};
}  // namespace neo::network::p2p
