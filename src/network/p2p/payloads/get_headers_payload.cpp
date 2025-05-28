#include <neo/network/p2p/payloads/get_headers_payload.h>

namespace neo::network::p2p::payloads
{
    GetHeadersPayload::GetHeadersPayload(const io::UInt256& hashStart, int16_t count)
        : GetBlocksPayload(hashStart, count)
    {
    }
    
    GetHeadersPayload GetHeadersPayload::Create(const io::UInt256& hashStart, int16_t count)
    {
        return GetHeadersPayload(hashStart, count);
    }
}
