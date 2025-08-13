/**
 * @file get_headers_payload.cpp
 * @brief Get Headers Payload
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/network/p2p/payloads/get_headers_payload.h>

namespace neo::network::p2p::payloads
{
GetHeadersPayload::GetHeadersPayload(const io::UInt256& hashStart, int16_t count) : GetBlocksPayload(hashStart)
{
    SetCount(count);
}

GetHeadersPayload GetHeadersPayload::Create(const io::UInt256& hashStart, int16_t count)
{
    return GetHeadersPayload(hashStart, count);
}
}  // namespace neo::network::p2p::payloads
