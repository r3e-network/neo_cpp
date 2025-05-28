#pragma once

#include <neo/network/p2p/payloads/inv_payload.h>

namespace neo::network::p2p::payloads
{
    /**
     * @brief Represents a getdata message payload.
     * 
     * This class is identical to InvPayload, but it's used for a different purpose.
     */
    class GetDataPayload : public InvPayload
    {
    public:
        /**
         * @brief Constructs an empty GetDataPayload.
         */
        GetDataPayload() = default;
        
        /**
         * @brief Constructs a GetDataPayload with the specified type and hashes.
         * @param type The type.
         * @param hashes The hashes.
         */
        GetDataPayload(InventoryType type, const std::vector<io::UInt256>& hashes)
            : InvPayload(type, hashes)
        {
        }
        
        /**
         * @brief Constructs a GetDataPayload with the specified inventory vectors.
         * @param inventories The inventory vectors.
         */
        explicit GetDataPayload(const std::vector<InventoryVector>& inventories)
            : InvPayload(inventories)
        {
        }
    };
}
