/**
 * @file ecrecover.h
 * @brief Ecrecover
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_span.h>

#include <optional>

namespace neo::cryptography
{
/**
 * @brief Recovers public key from signature (Ethereum-style ECRecover)
 * @param hash The hash that was signed (32 bytes)
 * @param signature The signature (64 bytes: r + s)
 * @param recovery_id Recovery ID (0-3)
 * @return The recovered public key, or null if recovery failed
 */
std::optional<ecc::ECPoint> ECRecover(const io::ByteSpan& hash, const io::ByteSpan& signature, uint8_t recovery_id);

}  // namespace neo::cryptography