/**
 * @file get_headers_payload.h
 * @brief Get Headers Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/network/p2p/payloads/get_blocks_payload.h>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents a get headers payload.
 *
 * This message is used to request headers from a peer.
 */
class GetHeadersPayload : public GetBlocksPayload
{
   public:
    /**
     * @brief Constructs a GetHeadersPayload.
     */
    GetHeadersPayload() = default;

    /**
     * @brief Constructs a GetHeadersPayload with the specified parameters.
     * @param hashStart The start hash.
     * @param count The count.
     */
    GetHeadersPayload(const io::UInt256& hashStart, int16_t count = -1);

    /**
     * @brief Creates a new GetHeadersPayload with the specified parameters.
     * @param hashStart The start hash.
     * @param count The count.
     * @return The created payload.
     */
    static GetHeadersPayload Create(const io::UInt256& hashStart, int16_t count = -1);
};
}  // namespace neo::network::p2p::payloads
