#include <neo/network/tcp_client.h>
#include <neo/logging/logger.h>
#include <boost/bind.hpp>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace neo::network
{
    TcpClient::TcpClient()
        : running_(false)
    {
        ioContext_ = std::make_unique<boost::asio::io_context>();
        ioContextPtr_ = ioContext_.get();

        // Create a work guard to keep the io_context running
        workGuard_ = std::make_unique<boost::asio::executor_work_guard<
            boost::asio::io_context::executor_type>>(
            ioContextPtr_->get_executor());

        // Start the io_context in a thread pool
        unsigned int thread_count = std::max(1u, std::thread::hardware_concurrency());

        for (unsigned int i = 0; i < thread_count; ++i)
        {
            ioThreads_.emplace_back([this, i]() {
                try
                {
                    logging::Logger::Instance().Debug("Network", "Client IO thread " + std::to_string(i) + " started");
                    ioContextPtr_->run();
                    logging::Logger::Instance().Debug("Network", "Client IO thread " + std::to_string(i) + " stopped");
                }
                catch (const std::exception& e)
                {
                    logging::Logger::Instance().Error("Network",
                        std::string("Error in client IO thread ") + std::to_string(i) + ": " + e.what());
                }
            });
        }

        running_ = true;
    }

    TcpClient::TcpClient(boost::asio::io_context& ioContext)
        : ioContextPtr_(&ioContext), running_(true)
    {
        // Using external io_context, no need to create threads
    }

    TcpClient::~TcpClient()
    {
        Stop();
    }

    std::shared_ptr<TcpConnection> TcpClient::Connect(const IPEndPoint& endpoint, uint32_t timeout)
    {
        if (!running_)
            throw std::runtime_error("TcpClient is not running");

        logging::Logger::Instance().Info("Network", "Connecting to " + endpoint.ToString());

        // Create a connection
        auto connection = std::make_shared<TcpConnection>(*ioContextPtr_);

        try
        {
            // Resolve the endpoint
            boost::asio::ip::tcp::endpoint asioEndpoint;
            if (endpoint.GetAddress().GetAddressLength() == 4)
            {
                // IPv4
                boost::asio::ip::address_v4::bytes_type bytes;
                std::memcpy(bytes.data(), endpoint.GetAddress().GetAddressBytes(), 4);
                asioEndpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(bytes), endpoint.GetPort());
            }
            else if (endpoint.GetAddress().GetAddressLength() == 16)
            {
                // IPv6
                boost::asio::ip::address_v6::bytes_type bytes;
                std::memcpy(bytes.data(), endpoint.GetAddress().GetAddressBytes(), 16);
                asioEndpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v6(bytes), endpoint.GetPort());
            }
            else
            {
                throw std::runtime_error("Invalid IP address");
            }

            // Connect with timeout
            if (timeout > 0)
            {
                // Set socket to non-blocking mode
                connection->GetSocket().non_blocking(true);

                // Start connect operation
                boost::system::error_code ec;
                connection->GetSocket().connect(asioEndpoint, ec);

                if (ec == boost::asio::error::would_block)
                {
                    // Connection in progress, wait for completion
                    connection->GetSocket().async_connect(asioEndpoint, [](const boost::system::error_code&) {});

                    // Wait for connection completion with timeout
                    boost::asio::ip::tcp::socket::wait_type waitType = boost::asio::ip::tcp::socket::wait_write;
                    if (connection->GetSocket().wait(waitType, ec) && !ec)
                    {
                        // Check if connection was successful
                        boost::system::error_code connectEc;
                        connection->GetSocket().remote_endpoint(connectEc);
                        if (connectEc)
                        {
                            throw std::runtime_error("Connection failed: " + connectEc.message());
                        }
                    }
                    else
                    {
                        throw std::runtime_error("Connection timed out or failed");
                    }
                }
                else if (ec)
                {
                    throw std::runtime_error("Connection failed: " + ec.message());
                }

                // Reset to blocking mode
                connection->GetSocket().non_blocking(false);
            }
            else
            {
                // Connect without timeout
                connection->GetSocket().connect(asioEndpoint);
            }

            // Add to connections map
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            std::string endpointStr = endpoint.ToString();
            connections_[endpointStr] = connection;

            // Set up connection closed callback
            connection->SetConnectionClosedCallback([this, endpointStr]() {
                HandleConnectionClosed(endpointStr);
            });

            // Start the connection
            connection->Start();

            logging::Logger::Instance().Info("Network", "Connected to " + endpoint.ToString());

            return connection;
        }
        catch (const std::exception& e)
        {
            logging::Logger::Instance().Error("Network",
                std::string("Failed to connect to ") + endpoint.ToString() + ": " + e.what());

            // Clean up the connection
            connection->Close();

            // Rethrow the exception
            throw;
        }
    }

    void TcpClient::ConnectAsync(const IPEndPoint& endpoint,
        std::function<void(std::shared_ptr<TcpConnection>, const std::error_code&)> callback)
    {
        if (!running_)
        {
            if (callback)
            {
                callback(nullptr, boost::asio::error::not_connected);
            }
            return;
        }

        logging::Logger::Instance().Info("Network", "Connecting asynchronously to " + endpoint.ToString());

        // Create a connection
        auto connection = std::make_shared<TcpConnection>(*ioContextPtr_);

        try
        {
            // Resolve the endpoint
            boost::asio::ip::tcp::endpoint asioEndpoint;
            if (endpoint.GetAddress().GetAddressLength() == 4)
            {
                // IPv4
                boost::asio::ip::address_v4::bytes_type bytes;
                std::memcpy(bytes.data(), endpoint.GetAddress().GetAddressBytes(), 4);
                asioEndpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(bytes), endpoint.GetPort());
            }
            else if (endpoint.GetAddress().GetAddressLength() == 16)
            {
                // IPv6
                boost::asio::ip::address_v6::bytes_type bytes;
                std::memcpy(bytes.data(), endpoint.GetAddress().GetAddressBytes(), 16);
                asioEndpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v6(bytes), endpoint.GetPort());
            }
            else
            {
                throw std::runtime_error("Invalid IP address");
            }

            // Start async connect
            connection->GetSocket().async_connect(asioEndpoint,
                [this, connection, endpoint, callback](const boost::system::error_code& error) {
                    if (!error)
                    {
                        // Connected successfully

                        // Add to connections map
                        std::lock_guard<std::mutex> lock(connectionsMutex_);
                        std::string endpointStr = endpoint.ToString();
                        connections_[endpointStr] = connection;

                        // Set up connection closed callback
                        connection->SetConnectionClosedCallback([this, endpointStr]() {
                            HandleConnectionClosed(endpointStr);
                        });

                        // Start the connection
                        connection->Start();

                        logging::Logger::Instance().Info("Network", "Connected asynchronously to " + endpoint.ToString());

                        // Invoke the callback
                        if (callback)
                        {
                            callback(connection, error);
                        }
                    }
                    else
                    {
                        // Connection failed
                        logging::Logger::Instance().Error("Network",
                            std::string("Failed to connect asynchronously to ") + endpoint.ToString() +
                            ": " + error.message());

                        // Invoke the callback with error
                        if (callback)
                        {
                            callback(nullptr, error);
                        }
                    }
                });
        }
        catch (const std::exception& e)
        {
            logging::Logger::Instance().Error("Network",
                std::string("Failed to start async connection to ") + endpoint.ToString() + ": " + e.what());

            // Invoke the callback with error
            if (callback)
            {
                callback(nullptr, boost::asio::error::fault);
            }
        }
    }

    void TcpClient::Stop()
    {
        if (!running_)
            return;

        logging::Logger::Instance().Info("Network", "Stopping TcpClient");
        running_ = false;

        // Close all connections
        {
            std::lock_guard<std::mutex> lock(connectionsMutex_);
            for (auto& [endpoint, connection] : connections_)
            {
                try {
                    connection->Close();
                } catch (const std::exception& e) {
                    logging::Logger::Instance().Warning("Network",
                        std::string("Error closing connection to ") + endpoint + ": " + e.what());
                }
            }
            connections_.clear();
        }

        // Only stop if we own the io_context
        if (ioContext_)
        {
            // Reset the work guard to allow the io_context to finish
            if (workGuard_)
            {
                workGuard_.reset();
            }

            // Stop the IO context
            ioContext_->stop();

            // Wait for all threads to finish
            for (auto& thread : ioThreads_)
            {
                if (thread.joinable())
                {
                    try {
                        thread.join();
                    } catch (const std::exception& e) {
                        logging::Logger::Instance().Warning("Network",
                            std::string("Error joining IO thread: ") + e.what());
                    }
                }
            }
            ioThreads_.clear();
        }

        logging::Logger::Instance().Info("Network", "TcpClient stopped");
    }

    void TcpClient::HandleConnectionClosed(const std::string& endpoint)
    {
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        connections_.erase(endpoint);

        logging::Logger::Instance().Info("Network", "Connection to " + endpoint + " closed");
    }
}
