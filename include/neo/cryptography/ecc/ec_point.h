/**
 * @file ec_point.h
 * @brief Ec Point
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

// This file is deprecated. Please use <neo/cryptography/ecc/ecpoint.h> instead
#include <neo/cryptography/ecc/ecpoint.h>

namespace neo::cryptography
{
// Alias ECPoint from ecc namespace for backward compatibility
using ECPoint = neo::cryptography::ecc::ECPoint;
}  // namespace neo::cryptography