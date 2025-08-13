/**
 * @file connection.cpp
 * @brief Connection
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/connection.h>

#include <chrono>

namespace neo::network::p2p
{
uint32_t Connection::nextId_ = 1;

Connection::Connection()
    : id_(nextId_++),
      lastMessageReceived_(0),
      lastMessageSent_(0),
      lastPingSent_(0),
      lastPingReceived_(0),
      pingTime_(0),
      bytesSent_(0),
      bytesReceived_(0),
      messagesSent_(0),
      messagesReceived_(0)
{
    // Initialize the last message received and sent times to the current time
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    lastMessageReceived_ = now;
    lastMessageSent_ = now;
}

uint32_t Connection::GetId() const { return id_; }

uint64_t Connection::GetLastMessageReceived() const { return lastMessageReceived_; }

uint64_t Connection::GetLastMessageSent() const { return lastMessageSent_; }

uint64_t Connection::GetLastPingSent() const { return lastPingSent_; }

uint64_t Connection::GetLastPingReceived() const { return lastPingReceived_; }

uint32_t Connection::GetPingTime() const { return pingTime_; }

uint64_t Connection::GetBytesSent() const { return bytesSent_; }

uint64_t Connection::GetBytesReceived() const { return bytesReceived_; }

uint64_t Connection::GetMessagesSent() const { return messagesSent_; }

uint64_t Connection::GetMessagesReceived() const { return messagesReceived_; }

void Connection::SetMessageReceivedCallback(std::function<void(const Message&)> callback)
{
    messageReceivedCallback_ = callback;
}

void Connection::SetDisconnectedCallback(std::function<void()> callback) { disconnectedCallback_ = callback; }

void Connection::OnMessageReceived(const Message& message)
{
    UpdateLastMessageReceived();
    UpdateMessagesReceived();

    if (messageReceivedCallback_)
    {
        messageReceivedCallback_(message);
    }
}

void Connection::OnDisconnected()
{
    if (disconnectedCallback_)
    {
        disconnectedCallback_();
    }
}

void Connection::UpdateLastMessageReceived()
{
    lastMessageReceived_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
}

void Connection::UpdateLastMessageSent()
{
    lastMessageSent_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
}

void Connection::UpdateLastPingSent()
{
    lastPingSent_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
}

void Connection::UpdateLastPingReceived()
{
    lastPingReceived_ =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
}

void Connection::UpdatePingTime(uint32_t pingTime) { pingTime_ = pingTime; }

void Connection::UpdateBytesSent(uint64_t bytes) { bytesSent_ += bytes; }

void Connection::UpdateBytesReceived(uint64_t bytes) { bytesReceived_ += bytes; }

void Connection::UpdateMessagesSent() { messagesSent_++; }

void Connection::UpdateMessagesReceived() { messagesReceived_++; }
}  // namespace neo::network::p2p
