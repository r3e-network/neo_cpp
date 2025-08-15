#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace neo::sdk {

// Common type aliases
using json = nlohmann::json;
using Bytes = std::vector<uint8_t>;

// Forward declarations to avoid circular dependencies
namespace wallet {
    class WalletManager;
    class Account;
}

namespace transaction {
    class Transaction;
    class TransactionManager;
}

namespace rpc {
    class RpcClient;
}

namespace contract {
    class NEP17Token;
}

// Common utility functions
namespace utils {
    // Convert bytes to hex string
    std::string BytesToHex(const Bytes& bytes);
    
    // Convert hex string to bytes
    Bytes HexToBytes(const std::string& hex);
    
    // Base58 encoding/decoding
    std::string Base58Encode(const Bytes& bytes);
    Bytes Base58Decode(const std::string& str);
    
    // Address validation
    bool IsValidAddress(const std::string& address);
}

} // namespace neo::sdk