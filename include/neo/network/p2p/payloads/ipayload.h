#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/uint256.h>

#include <cstddef>

namespace neo::network::p2p::payloads
{
/**
 * @brief Interface for network payload serialization.
 * This matches the C# IPayload interface exactly.
 */
class IPayload
{
   public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~IPayload() = default;

    /**
     * @brief Serializes the payload to a binary writer.
     * @param writer The binary writer to write to.
     */
    virtual void Serialize(io::BinaryWriter& writer) const = 0;

    /**
     * @brief Deserializes the payload from a binary reader.
     * @param reader The binary reader to read from.
     */
    virtual void Deserialize(io::BinaryReader& reader) = 0;

    /**
     * @brief Gets the hash of the payload.
     * @return The hash of the payload.
     */
    virtual io::UInt256 GetHash() const = 0;

    /**
     * @brief Gets the size of the payload in bytes.
     * @return The size of the payload.
     */
    virtual size_t GetSize() const = 0;
};

}  // namespace neo::network::p2p::payloads