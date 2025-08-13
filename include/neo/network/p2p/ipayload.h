/**
 * @file ipayload.h
 * @brief Ipayload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>

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
}  // namespace neo::network::p2p
