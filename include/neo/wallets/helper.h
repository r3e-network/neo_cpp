#pragma once

#include <neo/cryptography/hash.h>
#include <neo/cryptography/base58.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/vm/script_builder.h>

#include <cstring>
#include <string>
#include <vector>

namespace neo {
namespace wallets {

/**
 * @brief Helper functions for wallet operations
 */
class Helper {
public:
    /**
     * @brief Convert a script hash to an address
     * @param scriptHash The script hash to convert
     * @param addressVersion The address version byte (default 0x35 for Neo3 mainnet)
     * @return The Neo address string
     */
    static std::string ToAddress(const io::UInt160& scriptHash, uint8_t addressVersion = 0x35)
    {
        std::vector<uint8_t> data(1 + io::UInt160::Size);
        data[0] = addressVersion;
        std::memcpy(data.data() + 1, scriptHash.Data(), io::UInt160::Size);
        return cryptography::Base58::EncodeCheck(io::ByteSpan(data.data(), data.size()));
    }
    
    /**
     * @brief Convert an address to a script hash
     * @param address The Neo address string
     * @return The script hash
     */
    static io::UInt160 ToScriptHash(const std::string& address, uint8_t addressVersion = 0x35)
    {
        auto decoded = cryptography::Base58::DecodeCheck(address);
        if (decoded.size() != 1 + io::UInt160::Size)
        {
            throw std::invalid_argument("Invalid address format");
        }
        if (decoded[0] != addressVersion)
        {
            throw std::invalid_argument("Address version mismatch");
        }
        return io::UInt160(io::ByteSpan(decoded.data() + 1, io::UInt160::Size));
    }
    
    /**
     * @brief Validate a Neo address
     * @param address The address to validate
     * @return true if valid, false otherwise
     */
    static bool IsValidAddress(const std::string& address, uint8_t addressVersion = 0x35)
    {
        try
        {
            (void)ToScriptHash(address, addressVersion);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
    
    /**
     * @brief Create a script hash from a public key
     * @param publicKey The public key
     * @return The script hash
     */
    static io::ByteVector CreateSignatureRedeemScript(const std::vector<uint8_t>& publicKey)
    {
        vm::ScriptBuilder builder;
        builder.EmitPush(io::ByteSpan(publicKey.data(), publicKey.size()));
        builder.EmitSysCall("System.Crypto.CheckSig");
        return builder.ToArray();
    }
    
    /**
     * @brief Get the network version byte for an address
     * @param network The network name ("mainnet", "testnet", "private")
     * @return The version byte
     */
    static uint8_t GetAddressVersion(const std::string& network) {
        if (network == "mainnet") return 0x35;  // 'N' prefix
        if (network == "testnet") return 0x42;  // 'T' prefix  
        if (network == "private") return 0x00;  // Custom
        return 0x35; // Default to mainnet
    }
};

} // namespace wallets
} // namespace neo
