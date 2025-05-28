#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>

namespace neo::network::p2p
{
    /**
     * @brief Interface for message payloads.
     */
    class IPayload : public io::ISerializable, public io::IJsonSerializable
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IPayload() = default;
    };
}
