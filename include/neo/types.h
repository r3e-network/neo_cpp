#pragma once

/**
 * @file types.h
 * @brief Common type aliases for Neo C++ implementation
 *
 * This header provides namespace aliases to maintain compatibility
 * with the expected type names throughout the codebase.
 */

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

namespace neo
{
/**
 * @brief Type aliases for common Neo types
 *
 * These aliases provide compatibility with the expected type names
 * used throughout the codebase while maintaining the actual
 * implementation in the appropriate namespaces.
 */
namespace types
{
// Core integer types
using UInt160 = neo::io::UInt160;
using UInt256 = neo::io::UInt256;

// Byte handling types
using ByteVector = neo::io::ByteVector;
using ByteSpan = neo::io::ByteSpan;
}  // namespace types
}  // namespace neo
