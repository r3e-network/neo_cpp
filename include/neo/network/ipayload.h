#pragma once

#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>

namespace neo::network
{
/**
 * @brief Interface for message payloads.
 */
class IPayload : public io::ISerializable, public io::IJsonSerializable
{
  public:
    /**
     * @brief Destructor.
     */
    virtual ~IPayload() = default;
};
}  // namespace neo::network
