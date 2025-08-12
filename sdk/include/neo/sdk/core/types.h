#pragma once

#include <memory>
#include <vector>
#include <string>
#include <neo/io/uint256.h>
#include <neo/io/uint160.h>
#include <neo/cryptography/ecc/ec_point.h>
#include <neo/core/transaction.h>
#include <neo/ledger/block.h>
#include <neo/ledger/header.h>

namespace neo::sdk::core {

// Re-export core types from the node implementation
using UInt256 = neo::io::UInt256;
using UInt160 = neo::io::UInt160;
using ECPoint = neo::cryptography::ECPoint;
using Transaction = neo::core::Transaction;
using Block = neo::ledger::Block;
using Header = neo::ledger::Header;
using Witness = neo::core::Witness;
using Signer = neo::core::Signer;

// Additional SDK-specific types
struct UTXO {
    UInt256 txid;
    uint16_t vout;
    uint64_t value;
    UInt160 scriptHash;
};

struct Balance {
    std::string asset;
    uint64_t amount;
    uint32_t lastUpdatedBlock;
};

struct ContractParameter {
    enum Type {
        SIGNATURE,
        BOOLEAN,
        INTEGER,
        HASH160,
        HASH256,
        BYTE_ARRAY,
        PUBLIC_KEY,
        STRING,
        ARRAY,
        MAP,
        VOID
    };
    
    Type type;
    std::vector<uint8_t> value;
    
    static ContractParameter FromInteger(int64_t value);
    static ContractParameter FromString(const std::string& value);
    static ContractParameter FromAddress(const std::string& address);
    static ContractParameter FromHash160(const UInt160& hash);
    static ContractParameter FromHash256(const UInt256& hash);
    static ContractParameter FromBoolean(bool value);
    static ContractParameter FromByteArray(const std::vector<uint8_t>& value);
    static ContractParameter Null();
};

struct InvocationResult {
    std::string script;
    std::string state;
    uint64_t gasConsumed;
    std::vector<ContractParameter> stack;
    std::string exception;
};

struct TransactionAttribute {
    uint8_t usage;
    std::vector<uint8_t> data;
};

struct NetworkConfig {
    uint32_t magic;
    std::string name;
    std::vector<std::string> seedList;
    uint16_t defaultPort;
    uint32_t millisecondsPerBlock;
};

struct PeerInfo {
    std::string address;
    uint16_t port;
    std::string version;
    uint32_t lastSeen;
    uint32_t latency;
};

} // namespace neo::sdk::core