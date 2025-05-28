#include <neo/network/tcp_connection.h>
#include <neo/network/payload_factory.h>
#include <neo/logging/logger.h>
#include <neo/network/p2p/message_command.h>
#include <neo/network/p2p/message_flags.h>
#include <neo/cryptography/crypto.h>
#include <neo/io/binary_reader.h>
#include <sstream>
#include <stdexcept>

namespace neo::network
{
    TcpConnection::TcpConnection(boost::asio::io_context& ioContext)
        : socket_(ioContext), running_(false), sending_(false)
    {
    }

    TcpConnection::~TcpConnection()
    {
        Stop();
    }

    boost::asio::ip::tcp::socket& TcpConnection::GetSocket()
    {
        return socket_;
    }

    IPEndPoint TcpConnection::GetRemoteEndpoint() const
    {
        if (!socket_.is_open())
            throw std::runtime_error("Socket is not open");

        boost::asio::ip::tcp::endpoint endpoint = socket_.remote_endpoint();

        if (endpoint.address().is_v4())
        {
            boost::asio::ip::address_v4 address = endpoint.address().to_v4();
            uint8_t addressBytes[4];
            std::memcpy(addressBytes, address.to_bytes().data(), 4);
            return IPEndPoint(IPAddress(addressBytes, 4), endpoint.port());
        }
        else if (endpoint.address().is_v6())
        {
            boost::asio::ip::address_v6 address = endpoint.address().to_v6();
            uint8_t addressBytes[16];
            std::memcpy(addressBytes, address.to_bytes().data(), 16);
            return IPEndPoint(IPAddress(addressBytes, 16), endpoint.port());
        }
        else
        {
            throw std::runtime_error("Unknown address type");
        }
    }

    std::string TcpConnection::GetRemoteEndpointString() const
    {
        try {
            return GetRemoteEndpoint().ToString();
        } catch (const std::exception&) {
            return "Unknown";
        }
    }

    void TcpConnection::Start()
    {
        if (running_)
            return;

        running_ = true;

        // Set socket options
        boost::asio::ip::tcp::no_delay no_delay(true);
        socket_.set_option(no_delay);

        boost::asio::socket_base::keep_alive keep_alive(true);
        socket_.set_option(keep_alive);

        // Start reading
        ReadMessage();
    }

    void TcpConnection::Stop()
    {
        if (!running_)
            return;

        running_ = false;
        Close();
    }

    void TcpConnection::Close()
    {
        if (socket_.is_open())
        {
            try {
                socket_.close();
            } catch (const std::exception& e) {
                logging::Logger::Instance().Warning("Network",
                    std::string("Error closing socket: ") + e.what());
            }
        }

        // Notify connection closed
        if (connectionClosedCallback_)
        {
            try {
                connectionClosedCallback_();
            } catch (const std::exception& e) {
                logging::Logger::Instance().Warning("Network",
                    std::string("Error in connection closed callback: ") + e.what());
            }
        }
    }

    void TcpConnection::Send(const Message& message)
    {
        if (!running_ || !socket_.is_open())
            return;

        try
        {
            // Serialize the message
            io::ByteVector messageData = message.ToArray();

            // Queue the message
            sendQueue_.Push(std::vector<uint8_t>(messageData.Data(), messageData.Data() + messageData.Size()));

            // Process the queue if not already sending
            if (!sending_.exchange(true))
            {
                ProcessSendQueue();
            }
        }
        catch (const std::exception& e)
        {
            logging::Logger::Instance().Warning("Network",
                std::string("Error sending message: ") + e.what());
            Close();
        }
    }

    void TcpConnection::SetMessageReceivedCallback(std::function<void(const Message&)> callback)
    {
        messageReceivedCallback_ = std::move(callback);
    }

    void TcpConnection::SetConnectionClosedCallback(std::function<void()> callback)
    {
        connectionClosedCallback_ = std::move(callback);
    }

    void TcpConnection::ReadMessage()
    {
        if (!running_ || !socket_.is_open())
            return;

        // Read the header (24 bytes)
        socket_.async_read_some(boost::asio::buffer(receiveBuffer_, 24),
            std::bind(&TcpConnection::HandleReadHeader, shared_from_this(),
                std::placeholders::_1, std::placeholders::_2));
    }

    void TcpConnection::HandleReadHeader(const std::error_code& error, size_t bytesTransferred)
    {
        if (error || bytesTransferred < 24)
        {
            HandleError(error);
            return;
        }

        try
        {
            // Parse the header
            std::istringstream stream(std::string(reinterpret_cast<const char*>(receiveBuffer_), bytesTransferred));
            io::BinaryReader reader(stream);

            // Read magic (4 bytes)
            uint32_t magic = reader.ReadUInt32();

            // Read command (12 bytes)
            char commandBytes[12];
            io::ByteVector commandBytesVec = reader.ReadBytes(12);
            std::memcpy(commandBytes, commandBytesVec.Data(), 12);
            std::string commandStr;
            for (int i = 0; i < 12 && commandBytes[i] != 0; i++)
            {
                commandStr.push_back(commandBytes[i]);
            }

            // Convert string to command enum
            p2p::MessageCommand command;
            if (commandStr == "version") command = p2p::MessageCommand::Version;
            else if (commandStr == "verack") command = p2p::MessageCommand::Verack;
            else if (commandStr == "getaddr") command = p2p::MessageCommand::GetAddr;
            else if (commandStr == "addr") command = p2p::MessageCommand::Addr;
            else if (commandStr == "inv") command = p2p::MessageCommand::Inv;
            else if (commandStr == "getdata") command = p2p::MessageCommand::GetData;
            else if (commandStr == "getblocks") command = p2p::MessageCommand::GetBlocks;
            else if (commandStr == "getblockbyindex") command = p2p::MessageCommand::GetBlockByIndex;
            else if (commandStr == "getheaders") command = p2p::MessageCommand::GetHeaders;
            else if (commandStr == "tx") command = p2p::MessageCommand::Transaction;
            else if (commandStr == "block") command = p2p::MessageCommand::Block;
            else if (commandStr == "headers") command = p2p::MessageCommand::Headers;
            else if (commandStr == "ping") command = p2p::MessageCommand::Ping;
            else if (commandStr == "pong") command = p2p::MessageCommand::Pong;
            else if (commandStr == "merkleblock") command = p2p::MessageCommand::MerkleBlock;
            else if (commandStr == "filterload") command = p2p::MessageCommand::FilterLoad;
            else if (commandStr == "filteradd") command = p2p::MessageCommand::FilterAdd;
            else if (commandStr == "filterclear") command = p2p::MessageCommand::FilterClear;
            else if (commandStr == "reject") command = p2p::MessageCommand::Reject;
            else if (commandStr == "alert") command = p2p::MessageCommand::Alert;
            else if (commandStr == "extensible") command = p2p::MessageCommand::Extensible;
            else command = p2p::MessageCommand::Unknown;

            // Read payload size (4 bytes)
            uint32_t payloadSize = reader.ReadUInt32();

            // Read checksum (4 bytes)
            uint32_t checksum = reader.ReadUInt32();

            // Read flags
            p2p::MessageFlags flags = static_cast<p2p::MessageFlags>(reader.ReadUInt8());

            if (payloadSize == 0)
            {
                // No payload, create message directly
                Message message(command, io::ByteVector(), flags);

                // Notify listeners
                if (messageReceivedCallback_)
                {
                    try {
                        messageReceivedCallback_(message);
                    } catch (const std::exception& e) {
                        logging::Logger::Instance().Warning("Network",
                            std::string("Error in message received callback: ") + e.what());
                    }
                }

                // Continue reading
                ReadMessage();
            }
            else
            {
                // Read the payload
                ReadPayload(command, payloadSize, checksum, flags);
            }
        }
        catch (const std::exception& e)
        {
            logging::Logger::Instance().Warning("Network",
                std::string("Error parsing message header: ") + e.what());
            Close();
        }
    }

    void TcpConnection::ReadPayload(p2p::MessageCommand command, uint32_t payloadLength, uint32_t checksum, p2p::MessageFlags flags)
    {
        if (payloadLength > Message::PayloadMaxSize)
        {
            logging::Logger::Instance().Warning("Network",
                "Payload too large: " + std::to_string(payloadLength) + " bytes");
            Close();
            return;
        }

        // Read the payload
        boost::asio::async_read(socket_, boost::asio::buffer(receiveBuffer_, payloadLength),
            std::bind(&TcpConnection::HandleReadPayload, shared_from_this(),
                std::placeholders::_1, std::placeholders::_2, command, payloadLength, checksum, flags));
    }

    void TcpConnection::HandleReadPayload(const std::error_code& error, size_t bytesTransferred,
                                        p2p::MessageCommand command, uint32_t expectedLength, uint32_t checksum, p2p::MessageFlags flags)
    {
        if (error || bytesTransferred < expectedLength)
        {
            HandleError(error);
            return;
        }

        try
        {
            // Create a payload from the received data
            io::ByteVector payload(io::ByteSpan(receiveBuffer_, bytesTransferred));

            // Validate checksum
            if (!ValidateChecksum(payload, checksum))
            {
                logging::Logger::Instance().Warning("Network", "Invalid checksum");
                Close();
                return;
            }

            // Create message
            Message message(command, payload, flags);

            // Notify listeners
            if (messageReceivedCallback_)
            {
                try {
                    messageReceivedCallback_(message);
                } catch (const std::exception& e) {
                    logging::Logger::Instance().Warning("Network",
                        std::string("Error in message received callback: ") + e.what());
                }
            }

            // Continue reading
            ReadMessage();
        }
        catch (const std::exception& e)
        {
            logging::Logger::Instance().Warning("Network",
                std::string("Error handling payload: ") + e.what());
            Close();
        }
    }

    void TcpConnection::ProcessSendQueue()
    {
        if (!running_ || !socket_.is_open())
        {
            sending_ = false;
            return;
        }

        std::vector<uint8_t> data;
        if (sendQueue_.TryPop(data))
        {
            // Send the message
            boost::asio::async_write(socket_, boost::asio::buffer(data),
                std::bind(&TcpConnection::HandleWrite, shared_from_this(),
                    std::placeholders::_1, std::placeholders::_2));
        }
        else
        {
            // No more messages to send
            sending_ = false;
        }
    }

    void TcpConnection::HandleWrite(const std::error_code& error, size_t bytesTransferred)
    {
        if (error)
        {
            HandleError(error);
            return;
        }

        // Process the next message in the queue
        ProcessSendQueue();
    }

    void TcpConnection::HandleError(const std::error_code& error)
    {
        if (error.value() == boost::asio::error::operation_aborted)
        {
            // Socket was closed cleanly
            return;
        }

        logging::Logger::Instance().Warning("Network",
            "Socket error: " + error.message() + " (" + std::to_string(error.value()) + ")");
        Close();
    }

    bool TcpConnection::ValidateChecksum(const io::ByteVector& payload, uint32_t expectedChecksum)
    {
        // Calculate checksum
        uint32_t calculatedChecksum = 0;

        if (payload.Size() > 0)
        {
            auto hash = cryptography::Crypto::Hash256(payload.Data(), payload.Size());
            // First 4 bytes of the hash
            calculatedChecksum = static_cast<uint32_t>(hash[0]) |
                               (static_cast<uint32_t>(hash[1]) << 8) |
                               (static_cast<uint32_t>(hash[2]) << 16) |
                               (static_cast<uint32_t>(hash[3]) << 24);
        }

        return calculatedChecksum == expectedChecksum;
    }
}
