#include <boost/bind.hpp>
#include <chrono>
#include <neo/logging/logger.h>
#include <neo/network/tcp_server.h>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace neo::network
{
// TcpServer implementation
TcpServer::TcpServer(const IPEndPoint& endpoint, uint32_t maxConnections)
    : endpoint_(endpoint), running_(false), maxConnections_(maxConnections)
{
    ioContext_ = std::make_unique<boost::asio::io_context>();
}

TcpServer::~TcpServer()
{
    Stop();
}

void TcpServer::Start()
{
    if (running_)
    {
        neo::logging::Logger::Instance().Info("Network", "TcpServer already running");
        return;
    }

    neo::logging::Logger::Instance().Info("Network", "Starting TcpServer on " + endpoint_.ToString());

    try
    {
        ioContext_ = std::make_unique<boost::asio::io_context>();

        // Create the acceptor
        boost::asio::ip::tcp::endpoint endpoint;
        if (endpoint_.GetAddress().GetAddressLength() == 4)
        {
            // IPv4
            boost::asio::ip::address_v4::bytes_type bytes;
            std::memcpy(bytes.data(), endpoint_.GetAddress().GetAddressBytes(), 4);
            endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(bytes), endpoint_.GetPort());
        }
        else if (endpoint_.GetAddress().GetAddressLength() == 16)
        {
            // IPv6
            boost::asio::ip::address_v6::bytes_type bytes;
            std::memcpy(bytes.data(), endpoint_.GetAddress().GetAddressBytes(), 16);
            endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v6(bytes), endpoint_.GetPort());
        }
        else
        {
            throw std::runtime_error("Invalid IP address");
        }

        // Create acceptor with correct socket option settings
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(*ioContext_, endpoint, true);

        // Set socket options
        boost::asio::socket_base::reuse_address reuse_address(true);
        acceptor_->set_option(reuse_address);

        // Start accepting connections
        AcceptConnection();

        // Create a work guard to keep the io_context running
        workGuard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
            ioContext_->get_executor());

        // Start the io_context in a thread pool
        unsigned int thread_count = std::max(1u, std::thread::hardware_concurrency());
        neo::logging::Logger::Instance().Info("Network", "Starting " + std::to_string(thread_count) + " IO threads");

        for (unsigned int i = 0; i < thread_count; ++i)
        {
            ioThreads_.emplace_back(
                [this, i]()
                {
                    try
                    {
                        neo::logging::Logger::Instance().Debug("Network", "IO thread " + std::to_string(i) + " started");
                        ioContext_->run();
                        neo::logging::Logger::Instance().Debug("Network", "IO thread " + std::to_string(i) + " stopped");
                    }
                    catch (const std::exception& e)
                    {
                        // Log the error
                        neo::logging::Logger::Instance().Error("Network", std::string("Error in IO thread ") +
                                                                         std::to_string(i) + ": " + e.what());
                    }
                });
        }

        running_ = true;
        neo::logging::Logger::Instance().Info("Network", "TcpServer started successfully");
    }
    catch (const std::exception& e)
    {
        // Log the error
        neo::logging::Logger::Instance().Error("Network", std::string("Error starting server: ") + e.what());

        // Clean up
        Stop();

        // Rethrow the exception
        throw;
    }
}

void TcpServer::Stop()
{
    if (!running_)
    {
        neo::logging::Logger::Instance().Debug("Network", "TcpServer already stopped");
        return;
    }

    neo::logging::Logger::Instance().Info("Network", "Stopping TcpServer");
    running_ = false;

    // Close the acceptor
    if (acceptor_)
    {
        try
        {
            acceptor_->close();
            neo::logging::Logger::Instance().Debug("Network", "Acceptor closed");
        }
        catch (const std::exception& e)
        {
            neo::logging::Logger::Instance().Warning("Network", std::string("Error closing acceptor: ") + e.what());
        }
        acceptor_.reset();
    }

    // Close all connections
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        for (auto& [endpoint, connection] : connections_)
        {
            try
            {
                connection->Close();
            }
            catch (const std::exception& e)
            {
                neo::logging::Logger::Instance().Warning("Network", std::string("Error closing connection to ") + endpoint +
                                                                   ": " + e.what());
            }
        }
        size_t connectionCount = connections_.size();
        connections_.clear();
        neo::logging::Logger::Instance().Info("Network", "Closed " + std::to_string(connectionCount) + " connection(s)");
    }

    // Reset the work guard to allow the io_context to finish
    if (workGuard_)
    {
        workGuard_.reset();
        neo::logging::Logger::Instance().Debug("Network", "Work guard reset");
    }

    // Stop the IO context
    if (ioContext_)
    {
        try
        {
            ioContext_->stop();
            neo::logging::Logger::Instance().Debug("Network", "IO context stopped");
        }
        catch (const std::exception& e)
        {
            neo::logging::Logger::Instance().Warning("Network", std::string("Error stopping IO context: ") + e.what());
        }
    }

    // Wait for all threads to finish
    for (auto& thread : ioThreads_)
    {
        if (thread.joinable())
        {
            try
            {
                thread.join();
            }
            catch (const std::exception& e)
            {
                neo::logging::Logger::Instance().Warning("Network", std::string("Error joining IO thread: ") + e.what());
            }
        }
    }
    ioThreads_.clear();
    neo::logging::Logger::Instance().Debug("Network", "All IO threads joined");

    neo::logging::Logger::Instance().Info("Network", "TcpServer stopped successfully");
}

const IPEndPoint& TcpServer::GetEndpoint() const
{
    return endpoint_;
}

uint32_t TcpServer::GetConnectionCount() const
{
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    return static_cast<uint32_t>(connections_.size());
}

void TcpServer::SetConnectionAcceptedCallback(std::function<void(std::shared_ptr<TcpConnection>)> callback)
{
    connectionAcceptedCallback_ = std::move(callback);
}

void TcpServer::AcceptConnection()
{
    if (!running_)
        return;

    auto connection = std::make_shared<TcpConnection>(*ioContext_);
    acceptor_->async_accept(connection->GetSocket(), [this, connection](const boost::system::error_code& error)
                            { HandleAccept(connection, error); });
}

void TcpServer::HandleAccept(std::shared_ptr<TcpConnection> connection, const std::error_code& error)
{
    // Always accept the next connection regardless of any errors with the current one
    AcceptConnection();

    if (error)
    {
        neo::logging::Logger::Instance().Warning("Network", std::string("Error accepting connection: ") + error.message());
        return;
    }

    // Get remote endpoint
    IPEndPoint remoteEndpoint;
    try
    {
        remoteEndpoint = connection->GetRemoteEndpoint();
        std::string remoteEndpointStr = remoteEndpoint.ToString();
        neo::logging::Logger::Instance().Info("Network", "Accepted connection from " + remoteEndpointStr);
    }
    catch (const std::exception& e)
    {
        neo::logging::Logger::Instance().Warning("Network", std::string("Error getting remote endpoint: ") + e.what());
        connection->Close();
        return;
    }

    // Check if we have reached the connection limit
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        if (connections_.size() >= maxConnections_)
        {
            neo::logging::Logger::Instance().Warning("Network", "Connection limit reached, rejecting connection from " +
                                                               remoteEndpoint.ToString());
            connection->Close();
            return;
        }

        // Store the connection
        std::string endpointStr = remoteEndpoint.ToString();
        connections_[endpointStr] = connection;
    }

    // Set callbacks
    connection->SetConnectionClosedCallback([this, endpoint = remoteEndpoint.ToString()]()
                                            { RemoveConnection(endpoint); });

    // Start the connection
    connection->Start();

    // Notify listener
    if (connectionAcceptedCallback_)
    {
        try
        {
            connectionAcceptedCallback_(connection);
        }
        catch (const std::exception& e)
        {
            neo::logging::Logger::Instance().Warning("Network",
                                                std::string("Error in connection accepted callback: ") + e.what());
        }
    }
}

void TcpServer::RemoveConnection(const std::string& endpoint)
{
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    auto it = connections_.find(endpoint);
    if (it != connections_.end())
    {
        connections_.erase(it);
        neo::logging::Logger::Instance().Debug("Network", "Removed connection to " + endpoint);
    }
}

}  // namespace neo::network
