#pragma once

#include <cstdint>

namespace neo::network::p2p::payloads
{
/**
 * @brief Represents the response code for the oracle request.
 */
enum class OracleResponseCode : uint8_t
{
    /**
     * @brief Indicates that the request has been successfully completed.
     */
    Success = 0x00,

    /**
     * @brief Indicates that the protocol of the request is not supported.
     */
    ProtocolNotSupported = 0x10,

    /**
     * @brief Indicates that the oracle nodes cannot reach a consensus on the result of the request.
     */
    ConsensusUnreachable = 0x12,

    /**
     * @brief Indicates that the requested Uri does not exist.
     */
    NotFound = 0x14,

    /**
     * @brief Indicates that the request was not completed within the specified time.
     */
    Timeout = 0x16,

    /**
     * @brief Indicates that there is no permission to request the resource.
     */
    Forbidden = 0x18,

    /**
     * @brief Indicates that the data for the response is too large.
     */
    ResponseTooLarge = 0x1a,

    /**
     * @brief Indicates that the request failed due to insufficient balance.
     */
    InsufficientFunds = 0x1c,

    /**
     * @brief Indicates that the content-type of the request is not supported.
     */
    ContentTypeNotSupported = 0x1f,

    /**
     * @brief Indicates that the request failed due to other errors.
     */
    Error = 0xff
};
}  // namespace neo::network::p2p::payloads
