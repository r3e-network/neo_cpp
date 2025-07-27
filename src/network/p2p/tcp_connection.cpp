#include <neo/network/p2p/tcp_connection.h>
#include <neo/network/p2p/message.h>
#include <neo/io/byte_vector.h>
#include <neo/core/logging.h>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>

namespace neo::network::p2p
{
    std::shared_ptr<TcpConnection> TcpConnection::Create(boost::asio::ip::tcp::socket socket)
    {
        return std::make_shared<TcpConnection>(std::move(socket));
    }

    TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
        : socket_(std::move(socket)), connected_(true), receiveBuffer_(8192)
    {
    }

    TcpConnection::~TcpConnection()
    {
        Disconnect();
    }

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
        if (!connected_)
            return false;

        try
        {
            std::lock_guard<std::mutex> lock(sendMutex_);

            // Convert the message to a byte array
            io::ByteVector data = message.ToArray(enableCompression);

            // Send the data
            boost::asio::async_write(socket_, boost::asio::buffer(data.Data(), data.Size()),
                std::bind(&TcpConnection::HandleSend, shared_from_this(),
                    std::placeholders::_1, std::placeholders::_2));

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
        if (connected_)
        {
            DoReceive();
        }
    }

    void TcpConnection::DoReceive()
    {
        socket_.async_read_some(boost::asio::buffer(receiveBuffer_.Data(), receiveBuffer_.Size()),
            std::bind(&TcpConnection::HandleReceive, shared_from_this(),
                std::placeholders::_1, std::placeholders::_2));
    }

    void TcpConnection::HandleReceive(const std::error_code& error, std::size_t bytesTransferred)
    {
        if (error)
        {
            Disconnect();
            return;
        }

        if (bytesTransferred > 0)
        {
            // Update the bytes received
            UpdateBytesReceived(bytesTransferred);

            // Try to deserialize a message
            Message message;
            uint32_t bytesRead = Message::TryDeserialize(io::ByteSpan(receiveBuffer_.Data(), bytesTransferred), message);

            if (bytesRead > 0)
            {
                // Process the message
                OnMessageReceived(message);

                // If there are more bytes, process them
                if (bytesRead < bytesTransferred)
                {
                    // Move the remaining bytes to the beginning of the buffer
                    std::memmove(receiveBuffer_.Data(), receiveBuffer_.Data() + bytesRead, bytesTransferred - bytesRead);

                    // Try to deserialize another message
                    Message anotherMessage;
                    uint32_t anotherBytesRead = Message::TryDeserialize(io::ByteSpan(receiveBuffer_.Data(), bytesTransferred - bytesRead), anotherMessage);

                    if (anotherBytesRead > 0)
                    {
                        // Process the message
                        OnMessageReceived(anotherMessage);
                    }
                }
            }

            // Continue receiving
            if (connected_)
            {
                DoReceive();
            }
        }
    }

    void TcpConnection::HandleSend(const std::error_code& error, std::size_t bytesTransferred)
    {
        if (error)
        {
            Disconnect();
        }
    }
}
