/**
 * @file local_node_connection.h
 * @brief Local Node Connection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <memory>
#include <string>

#include "neo/network/p2p/remote_node.h"

namespace neo::network::p2p
{

/**
 * @brief Represents a local node connection in the P2P network
 */
class LocalNodeConnection
{
   public:
    LocalNodeConnection() = default;
    virtual ~LocalNodeConnection() = default;

    // Connection management
    virtual bool Connect(const std::string& address, uint16_t port) = 0;
    virtual bool Disconnect() = 0;
    virtual bool IsConnected() const = 0;

    // Message handling
    virtual bool SendMessage(const std::shared_ptr<Message>& message) = 0;
    virtual void SetMessageHandler(std::function<void(const std::shared_ptr<Message>&)> handler) = 0;

    // Connection info
    virtual std::string GetRemoteEndpoint() const = 0;
    virtual uint32_t GetLastActivityTime() const = 0;
};

}  // namespace neo::network::p2p