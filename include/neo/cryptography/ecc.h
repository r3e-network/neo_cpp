/**
 * @file ecc.h
 * @brief Elliptic curve cryptography
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

// Include the actual ECC implementations from the ecc subdirectory
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/cryptography/ecc/secp256r1.h>

// Create namespace aliases for compatibility
namespace neo::cryptography
{
// Alias the ecc namespace classes for backward compatibility
using ECPoint = ecc::ECPoint;
using KeyPair = ecc::KeyPair;
using Secp256r1 = ecc::Secp256r1;
}  // namespace neo::cryptography
