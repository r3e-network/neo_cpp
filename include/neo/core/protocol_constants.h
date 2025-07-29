#pragma once

#include <cstdint>

namespace neo::core
{

/**
 * @brief Protocol constants for Neo blockchain
 *
 * These constants define various limits and parameters used throughout
 * the Neo blockchain implementation for consistency with the C# version.
 */
class ProtocolConstants
{
  public:
    // Network and protocol limits
    static constexpr uint32_t MaxTransactionSize = 2000000;      // 2MB max transaction size
    static constexpr uint32_t MaxTransactionAttributes = 1024;   // Max attributes per transaction
    static constexpr uint8_t MaxTransactionWitnesses = 16;       // Max witnesses per transaction
    static constexpr uint32_t MaxContractParametersCount = 255;  // Max contract parameters
    static constexpr uint32_t MaxStackSize = 65536;              // Max VM stack size
    static constexpr uint32_t MaxScriptLength = 1024 * 1024;     // Max script length (1MB)
    static constexpr uint32_t MaxArraySize = 1024;               // Max array size in VM
    static constexpr uint32_t MaxStringLength = 65536;           // Max string length
    static constexpr uint32_t MaxByteArrayLength = 65536;        // Max byte array length
    static constexpr uint32_t MaxKeySize = 64;                   // Max storage key size
    static constexpr uint32_t MaxValueSize = 65536;              // Max storage value size

    // Block and transaction limits
    static constexpr uint32_t MaxBlockSize = 262144;               // 256KB max block size
    static constexpr uint32_t MaxTransactionsPerBlock = 512;       // Max transactions per block
    static constexpr uint32_t SecondsPerBlock = 15;                // Target block time in seconds
    static constexpr uint32_t BlocksPerDay = 5760;                 // Blocks per day (24*60*60/15)
    static constexpr uint32_t MaxValidUntilBlockIncrement = 5760;  // Max valid until block increment

    // Gas and fee constants
    static constexpr int64_t GasPerByte = 1000;             // Gas cost per byte
    static constexpr int64_t ApplicationGas = 10000000;     // 10 GAS in datoshi
    static constexpr int64_t SystemGas = 10000000;          // 10 GAS in datoshi
    static constexpr int64_t NetworkFeePerByte = 1000;      // Network fee per byte
    static constexpr int64_t MaxSystemFee = 10000000000LL;  // Max system fee (10,000 GAS)
    static constexpr int64_t MaxNetworkFee = 100000000;     // Max network fee (100 GAS)

    // Cryptographic constants
    static constexpr uint32_t ECPointSize = 33;              // Compressed EC point size
    static constexpr uint32_t ECPointUncompressedSize = 65;  // Uncompressed EC point size
    static constexpr uint32_t UInt160Size = 20;              // UInt160 size in bytes
    static constexpr uint32_t UInt256Size = 32;              // UInt256 size in bytes
    static constexpr uint32_t ScriptHashSize = 20;           // Script hash size
    static constexpr uint32_t SignatureSize = 64;            // Signature size
    static constexpr uint32_t PrivateKeySize = 32;           // Private key size

    // BLS12-381 constants
    static constexpr uint32_t BLS12_381_FieldElementSize = 48;  // BLS12-381 field element size
    static constexpr uint32_t BLS12_381_G1PointSize = 48;       // BLS12-381 G1 point size
    static constexpr uint32_t BLS12_381_G2PointSize = 96;       // BLS12-381 G2 point size
    static constexpr uint32_t BLS12_381_GTElementSize = 384;    // BLS12-381 GT element size
    static constexpr uint32_t BLS12_381_ScalarSize = 32;        // BLS12-381 scalar size

    // P2P Network constants
    static constexpr uint16_t DefaultP2PPort = 10333;           // Default P2P port (mainnet)
    static constexpr uint16_t DefaultTestnetP2PPort = 20333;    // Default testnet P2P port
    static constexpr uint16_t DefaultRPCPort = 10332;           // Default RPC port (mainnet)
    static constexpr uint16_t DefaultTestnetRPCPort = 20332;    // Default testnet RPC port
    static constexpr uint32_t MaxPeers = 100;                   // Maximum number of peers
    static constexpr uint32_t MaxPayloadSize = 0x02000000;      // Max P2P payload size (32MB)
    static constexpr uint32_t MessageHeaderSize = 24;           // P2P message header size
    static constexpr uint32_t MainnetNetworkMagic = 860833102;  // Mainnet network magic number
    static constexpr uint8_t AddressVersion = 0x35;             // Neo address version byte (53 decimal)

    // Consensus constants
    static constexpr uint32_t ConsensusNodeCount = 7;    // Default consensus node count
    static constexpr uint32_t CommitteeSize = 21;        // Committee size
    static constexpr uint32_t ConsensusTimeout = 15000;  // Consensus timeout in ms
    static constexpr uint32_t ViewChangeFactor = 2;      // View change factor

    // Contract constants
    static constexpr uint8_t MaxSubitems = 16;           // Max subitems in contract parameters
    static constexpr uint32_t MaxManifestSize = 65536;   // Max contract manifest size
    static constexpr uint32_t MaxMethodNameLength = 32;  // Max contract method name length
    static constexpr uint32_t MaxEventNameLength = 32;   // Max contract event name length

    // Storage constants
    static constexpr uint8_t StorageKeyPrefixLength = 1;      // Storage key prefix length
    static constexpr uint32_t MaxStorageKeyLength = 64;       // Max storage key length
    static constexpr uint32_t MaxStorageValueLength = 65535;  // Max storage value length

    // Oracle constants
    static constexpr uint32_t MaxOracleURLLength = 256;        // Max oracle URL length
    static constexpr uint32_t MaxOracleRequestGas = 50000000;  // Max gas for oracle request
    static constexpr uint32_t OracleResponseGas = 10000000;    // Gas cost for oracle response

    // VM execution limits
    static constexpr uint32_t MaxInvocationStackSize = 1024;  // Max invocation stack size
    static constexpr uint32_t MaxItemSize = 1024 * 1024;      // Max size of a single stack item
    static constexpr uint32_t MaxCloneDepth = 255;            // Max clone depth for reference types

  private:
    // Private constructor to prevent instantiation
    ProtocolConstants() = delete;
};

// Convenience aliases for commonly used constants
using PC = ProtocolConstants;

}  // namespace neo::core