#pragma once

#include <neo/io/uint160.h>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/hash.h>
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
    static std::string ToAddress(const io::UInt160& scriptHash, uint8_t addressVersion = 0x35) {
        // Neo address format: version + scriptHash + checksum
        std::vector<uint8_t> data;
        data.push_back(addressVersion);
        
        // Add script hash bytes
        auto hashBytes = scriptHash.ToArray();
        data.insert(data.end(), hashBytes.begin(), hashBytes.end());
        
        // Calculate checksum (double SHA256)
        auto hash1 = cryptography::Hash::Sha256(io::ByteSpan(data.data(), data.size()));
        auto hash2 = cryptography::Hash::Sha256(io::ByteSpan(hash1.ToArray().Data(), 32));
        
        // Add first 4 bytes of checksum
        auto checksumBytes = hash2.ToArray();
        data.insert(data.end(), checksumBytes.begin(), checksumBytes.begin() + 4);
        
        // Encode to Base58
        return cryptography::Base58::Encode(data);
    }
    
    /**
     * @brief Convert an address to a script hash
     * @param address The Neo address string
     * @return The script hash
     */
    static io::UInt160 ToScriptHash(const std::string& address) {
        // Decode from Base58
        auto decoded = cryptography::Base58::Decode(address);
        
        if (decoded.size() != 25) { // 1 version + 20 hash + 4 checksum
            throw std::invalid_argument("Invalid address length");
        }
        
        // Extract script hash (skip version byte, take 20 bytes)
        std::vector<uint8_t> scriptHashBytes(decoded.begin() + 1, decoded.begin() + 21);
        
        // Verify checksum
        std::vector<uint8_t> dataToCheck(decoded.begin(), decoded.begin() + 21);
        auto hash1 = cryptography::Hash::Sha256(io::ByteSpan(dataToCheck.data(), dataToCheck.size()));
        auto hash2 = cryptography::Hash::Sha256(io::ByteSpan(hash1.ToArray().Data(), 32));
        
        // Check if last 4 bytes match checksum
        for (int i = 0; i < 4; i++) {
            if (decoded[21 + i] != hash2.ToArray()[i]) {
                throw std::invalid_argument("Invalid address checksum");
            }
        }
        
        return io::UInt160(scriptHashBytes.data());
    }
    
    /**
     * @brief Validate a Neo address
     * @param address The address to validate
     * @return true if valid, false otherwise
     */
    static bool IsValidAddress(const std::string& address) {
        try {
            if (address.length() != 34) return false;
            if (address[0] != 'N' && address[0] != 'A') return false;
            
            // Try to decode and verify checksum
            ToScriptHash(address);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Create a script hash from a public key
     * @param publicKey The public key
     * @return The script hash
     */
    static io::UInt160 CreateSignatureRedeemScript(const std::vector<uint8_t>& publicKey) {
        // OpCode.PUSH1 (0x51) + publicKey + OpCode.SYSCALL (0x41) + InteropService.Crypto.CheckSig
        std::vector<uint8_t> script;
        
        // Add PUSH opcode for public key length
        script.push_back(static_cast<uint8_t>(publicKey.size()));
        
        // Add public key
        script.insert(script.end(), publicKey.begin(), publicKey.end());
        
        // Add SYSCALL
        script.push_back(0x41);
        
        // Add CheckSig interop hash (4 bytes)
        // Neo.Crypto.CheckSig = 0x0a0b0c0d (example)
        script.push_back(0x0a);
        script.push_back(0x0b);
        script.push_back(0x0c);
        script.push_back(0x0d);
        
        // Hash the script with SHA256 and then RIPEMD160
        auto sha256Hash = cryptography::Hash::Sha256(io::ByteSpan(script.data(), script.size()));
        auto ripemd160Hash = cryptography::Hash::Ripemd160(io::ByteSpan(sha256Hash.ToArray().Data(), 32));
        
        return ripemd160Hash;
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