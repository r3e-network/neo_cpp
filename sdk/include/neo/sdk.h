#pragma once

/**
 * @file sdk.h
 * @brief Main header file for Neo C++ SDK
 * @version 1.0.0
 * 
 * Neo C++ SDK provides a comprehensive library for building applications
 * on the Neo blockchain using C++.
 */

// Core types and blockchain interface
#include <neo/sdk/core/types.h>
#include <neo/sdk/core/blockchain.h>

// Wallet management
#include <neo/sdk/wallet/wallet.h>
#include <neo/sdk/wallet/account.h>

// Transaction building
#include <neo/sdk/tx/transaction_builder.h>

// Smart contracts
#include <neo/sdk/contract/contract_deployer.h>
#include <neo/sdk/contract/contract_invoker.h>
#include <neo/sdk/contract/nep17_token.h>

// RPC client
#include <neo/sdk/rpc/rpc_client.h>

// Network client
#include <neo/sdk/network/network_client.h>

// Cryptography
#include <neo/sdk/crypto/keypair.h>
#include <neo/sdk/crypto/hash.h>

// Storage
#include <neo/sdk/storage/blockchain_storage.h>

// Utilities
#include <neo/sdk/utils/converter.h>

/**
 * @namespace neo::sdk
 * @brief Main namespace for Neo C++ SDK
 */
namespace neo::sdk {

/**
 * @brief SDK version information
 */
struct Version {
    static constexpr int MAJOR = 1;
    static constexpr int MINOR = 0;
    static constexpr int PATCH = 0;
    static constexpr const char* STRING = "1.0.0";
};

/**
 * @brief Initialize the SDK
 * @param config Configuration options
 * @return true if initialization successful
 */
bool Initialize(const std::string& config = "");

/**
 * @brief Shutdown the SDK and cleanup resources
 */
void Shutdown();

/**
 * @brief Get SDK version string
 * @return Version string in format "major.minor.patch"
 */
inline const char* GetVersion() {
    return Version::STRING;
}

} // namespace neo::sdk