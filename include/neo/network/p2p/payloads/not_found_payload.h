#pragma once

#include <neo/network/p2p/payloads/inv_payload.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a not found payload.
 *
 * This payload is sent in response to GetData messages when the requested
 * inventories are not found. It uses the same structure as InvPayload.
 */
class NotFoundPayload : public InvPayload
{
  public:
    /**
     * @brief Constructs an empty NotFoundPayload.
     */
    NotFoundPayload() = default;

    /**
     * @brief Constructs a NotFoundPayload with the specified inventory type and hashes.
     * @param type The inventory type.
     * @param hashes The hashes.
     */
    NotFoundPayload(InventoryType type, const std::vector<io::UInt256>& hashes) : InvPayload(type, hashes) {}

    /**
     * @brief Creates a NotFoundPayload from an inventory payload.
     * @param payload The inventory payload.
     * @return The not found payload.
     */
    static NotFoundPayload FromInvPayload(const InvPayload& payload)
    {
        return NotFoundPayload(payload.GetType(), payload.GetHashes());
    }
};
}  // namespace neo::network::p2p::payloads