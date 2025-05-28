#pragma once

#include <neo/network/p2p/ipayload.h>
#include <neo/network/p2p/message_command.h>
#include <neo/io/binary_reader.h>
#include <memory>

namespace neo::network::p2p
{
    /**
     * @brief Factory for creating payloads based on message command.
     */
    class PayloadFactory
    {
    public:
        /**
         * @brief Creates an empty payload based on the message command.
         * @param command The message command.
         * @return The empty payload or nullptr if the command doesn't have an associated payload.
         */
        static std::shared_ptr<IPayload> Create(MessageCommand command);
        
        /**
         * @brief Deserializes a payload based on the message command.
         * @param command The message command.
         * @param reader The binary reader.
         * @return The payload.
         */
        static std::shared_ptr<IPayload> DeserializePayload(MessageCommand command, io::BinaryReader& reader);
    };
}
