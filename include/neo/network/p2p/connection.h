#pragma once

#include <neo/network/p2p/message.h>
#include <neo/network/ip_endpoint.h>
#include <neo/io/byte_vector.h>
#include <functional>
#include <memory>
#include <string>
#include <cstdint>

namespace neo::network::p2p
{
    /**
     * @brief Represents a connection to a remote node.
     */
    class Connection
    {
    public:
        /**
         * @brief Constructs a Connection.
         */
        Connection();

        /**
         * @brief Virtual destructor.
         */
        virtual ~Connection() = default;

        /**
         * @brief Gets the remote endpoint.
         * @return The remote endpoint.
         */
        virtual IPEndPoint GetRemoteEndPoint() const = 0;

        /**
         * @brief Gets the local endpoint.
         * @return The local endpoint.
         */
        virtual IPEndPoint GetLocalEndPoint() const = 0;

        /**
         * @brief Gets the connection ID.
         * @return The connection ID.
         */
        uint32_t GetId() const;

        /**
         * @brief Gets the time of the last message received.
         * @return The time of the last message received.
         */
        uint64_t GetLastMessageReceived() const;

        /**
         * @brief Gets the time of the last message sent.
         * @return The time of the last message sent.
         */
        uint64_t GetLastMessageSent() const;

        /**
         * @brief Gets the time of the last ping sent.
         * @return The time of the last ping sent.
         */
        uint64_t GetLastPingSent() const;

        /**
         * @brief Gets the time of the last ping received.
         * @return The time of the last ping received.
         */
        uint64_t GetLastPingReceived() const;

        /**
         * @brief Gets the ping time.
         * @return The ping time.
         */
        uint32_t GetPingTime() const;

        /**
         * @brief Gets the number of bytes sent.
         * @return The number of bytes sent.
         */
        uint64_t GetBytesSent() const;

        /**
         * @brief Gets the number of bytes received.
         * @return The number of bytes received.
         */
        uint64_t GetBytesReceived() const;

        /**
         * @brief Gets the number of messages sent.
         * @return The number of messages sent.
         */
        uint64_t GetMessagesSent() const;

        /**
         * @brief Gets the number of messages received.
         * @return The number of messages received.
         */
        uint64_t GetMessagesReceived() const;

        /**
         * @brief Sends a message to the remote node.
         * @param message The message to send.
         * @param enableCompression Whether to enable compression.
         * @return True if the message was sent successfully, false otherwise.
         */
        virtual bool Send(const Message& message, bool enableCompression = true) = 0;

        /**
         * @brief Disconnects from the remote node.
         */
        virtual void Disconnect() = 0;

        /**
         * @brief Sets the message received callback.
         * @param callback The callback.
         */
        void SetMessageReceivedCallback(std::function<void(const Message&)> callback);

        /**
         * @brief Sets the disconnected callback.
         * @param callback The callback.
         */
        void SetDisconnectedCallback(std::function<void()> callback);

        /**
         * @brief Updates the ping time.
         * @param pingTime The ping time.
         */
        void UpdatePingTime(uint32_t pingTime);

    protected:
        /**
         * @brief Called when a message is received.
         * @param message The message.
         */
        void OnMessageReceived(const Message& message);

        /**
         * @brief Called when the connection is disconnected.
         */
        void OnDisconnected();

        /**
         * @brief Updates the last message received time.
         */
        void UpdateLastMessageReceived();

        /**
         * @brief Updates the last message sent time.
         */
        void UpdateLastMessageSent();

        /**
         * @brief Updates the last ping sent time.
         */
        void UpdateLastPingSent();

        /**
         * @brief Updates the last ping received time.
         */
        void UpdateLastPingReceived();

        /**
         * @brief Updates the bytes sent.
         * @param bytes The number of bytes sent.
         */
        void UpdateBytesSent(uint64_t bytes);

        /**
         * @brief Updates the bytes received.
         * @param bytes The number of bytes received.
         */
        void UpdateBytesReceived(uint64_t bytes);

        /**
         * @brief Updates the messages sent.
         */
        void UpdateMessagesSent();

        /**
         * @brief Updates the messages received.
         */
        void UpdateMessagesReceived();

    private:
        uint32_t id_;
        uint64_t lastMessageReceived_;
        uint64_t lastMessageSent_;
        uint64_t lastPingSent_;
        uint64_t lastPingReceived_;
        uint32_t pingTime_;
        uint64_t bytesSent_;
        uint64_t bytesReceived_;
        uint64_t messagesSent_;
        uint64_t messagesReceived_;
        std::function<void(const Message&)> messageReceivedCallback_;
        std::function<void()> disconnectedCallback_;

        static uint32_t nextId_;
    };
}
