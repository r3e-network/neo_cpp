/**
 * @file tcp_connection.cpp
 * @brief TCP network connections
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/core/logging.h>
#include <neo/io/byte_vector.h>
#include <neo/network/p2p/message.h>
#include <neo/network/p2p/tcp_connection.h>

#include <atomic>
#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

namespace neo::network::p2p
{
std::shared_ptr<TcpConnection> TcpConnection::Create(boost::asio::ip::tcp::socket socket)
{
    return std::make_shared<TcpConnection>(std::move(socket));
}

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)), connected_(true), receiveBuffer_(16384)
{
}

TcpConnection::~TcpConnection() { Disconnect(); }

IPEndPoint TcpConnection::GetRemoteEndPoint() const
{
    try
    {
        auto endpoint = socket_.remote_endpoint();
        return IPEndPoint(IPAddress(endpoint.address().to_string()), endpoint.port());
    }
    catch (const std::exception&)
    {
        return IPEndPoint();
    }
}

IPEndPoint TcpConnection::GetLocalEndPoint() const
{
    try
    {
        auto endpoint = socket_.local_endpoint();
        return IPEndPoint(IPAddress(endpoint.address().to_string()), endpoint.port());
    }
    catch (const std::exception&)
    {
        return IPEndPoint();
    }
}

bool TcpConnection::Send(const Message& message, bool enableCompression)
{
    if (!connected_) return false;

    try
    {
        std::lock_guard<std::mutex> lock(sendMutex_);

        // Convert the message to a byte array - should now resolve to correct p2p Message ToArray
        LOG_INFO("TcpConnection::Send - Calling ToArray on message");
        io::ByteVector data = message.ToArray(enableCompression);
        LOG_INFO("TcpConnection::Send - ToArray returned " + std::to_string(data.Size()) + " bytes");

        // Log outgoing message bytes for debugging
        std::string hexBytes;
        for (size_t i = 0; i < std::min(data.Size(), size_t(32)); ++i)
        {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02x ", data.Data()[i]);
            hexBytes += hex;
        }
        LOG_INFO("Sending message with " + std::to_string(data.Size()) + " bytes, first 32: " + hexBytes);

        // Send the data
        boost::asio::async_write(
            socket_, boost::asio::buffer(data.Data(), data.Size()),
            std::bind(&TcpConnection::HandleSend, shared_from_this(), std::placeholders::_1, std::placeholders::_2));

        // Update the last message sent time
        UpdateLastMessageSent();
        UpdateMessagesSent();
        UpdateBytesSent(data.Size());

        return true;
    }
    catch (const std::exception&)
    {
        Disconnect();
        return false;
    }
}

void TcpConnection::Disconnect()
{
    if (connected_.exchange(false))
    {
        LOG_DEBUG("TcpConnection::Disconnect called - closing socket");
        try
        {
            socket_.close();
        }
        catch (const std::exception& e)
        {
            // Log the error but don't prevent disconnection
            LOG_WARNING("Error while closing socket: {}", e.what());
        }

        OnDisconnected();
    }
}

void TcpConnection::StartReceiving()
{
    LOG_DEBUG("TcpConnection::StartReceiving called");
    if (connected_)
    {
        LOG_DEBUG("Starting to receive messages on connected socket");
        DoReceive();
    }
    else
    {
        LOG_WARNING("Cannot start receiving - socket not connected");
    }
}

void TcpConnection::DoReceive()
{
    if (!connected_) return;

    constexpr size_t kMinSlack = 4096;
    if (receiveBuffer_.Size() - bufferedBytes_ < kMinSlack)
    {
        receiveBuffer_.Resize(bufferedBytes_ + kMinSlack);
    }

    socket_.async_read_some(
        boost::asio::buffer(receiveBuffer_.Data() + bufferedBytes_, receiveBuffer_.Size() - bufferedBytes_),
        std::bind(&TcpConnection::HandleReceive, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
}

void TcpConnection::HandleReceive(const std::error_code& error, std::size_t bytesTransferred)
{
    if (error)
    {
        LOG_DEBUG("HandleReceive error: " + error.message());
        Disconnect();
        return;
    }

    LOG_DEBUG("Received " + std::to_string(bytesTransferred) + " bytes");

    if (bytesTransferred == 0)
    {
        DoReceive();
        return;
    }

    UpdateBytesReceived(bytesTransferred);
    bufferedBytes_ += bytesTransferred;

    // Log raw received bytes for debugging
    std::string hexBytes;
    for (size_t i = 0; i < std::min(bufferedBytes_, size_t(32)); ++i)
    {
        char hex[4];
        snprintf(hex, sizeof(hex), "%02x ", receiveBuffer_.Data()[i]);
        hexBytes += hex;
    }
    LOG_DEBUG("Buffer (first 32 bytes): " + hexBytes);

    while (bufferedBytes_ > 0)
    {
        Message message;
        uint32_t bytesRead = Message::TryDeserialize(io::ByteSpan(receiveBuffer_.Data(), bufferedBytes_), message);

        if (bytesRead == 0)
        {
            // Need more data
            break;
        }

        LOG_INFO("Deserialized message, command: {}", static_cast<int>(message.GetCommand()));
        OnMessageReceived(message);

        if (bytesRead < bufferedBytes_)
        {
            std::memmove(receiveBuffer_.Data(), receiveBuffer_.Data() + bytesRead, bufferedBytes_ - bytesRead);
        }
        bufferedBytes_ -= bytesRead;
    }

    if (connected_)
    {
        DoReceive();
    }
}

void TcpConnection::HandleSend(const std::error_code& error, std::size_t bytesTransferred)
{
    if (error)
    {
        LOG_DEBUG("HandleSend error: " + error.message());
        Disconnect();
    }
    else
    {
        LOG_DEBUG("Successfully sent " + std::to_string(bytesTransferred) + " bytes");
    }
}
}  // namespace neo::network::p2p
