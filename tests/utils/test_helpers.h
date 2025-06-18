#pragma once

#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <memory>
#include "neo/io/uint160.h"
#include "neo/io/uint256.h"
#include "neo/protocol_settings.h"
#include "neo/cryptography/ecc.h"

namespace neo::tests {

class TestHelpers {
public:
    // Random data generation
    static std::vector<uint8_t> GenerateRandomBytes(size_t length) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<uint8_t> dis(0, 255);
        
        std::vector<uint8_t> result(length);
        for (size_t i = 0; i < length; ++i) {
            result[i] = dis(gen);
        }
        return result;
    }
    
    static io::UInt160 GenerateRandomScriptHash() {
        auto bytes = GenerateRandomBytes(20);
        return io::UInt160(bytes);
    }
    
    static io::UInt256 GenerateRandomHash() {
        auto bytes = GenerateRandomBytes(32);
        return io::UInt256(bytes);
    }
    
    // Time utilities
    static uint64_t GetCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }
    
    // Protocol settings
    static std::shared_ptr<ProtocolSettings> GetDefaultSettings() {
        auto settings = std::make_shared<ProtocolSettings>();
        // Set default values for testing
        return settings;
    }
    
    // Cryptography helpers
    static std::vector<uint8_t> CreateVerificationScript(const cryptography::ECPoint& public_key) {
        // Simple verification script for testing
        std::vector<uint8_t> script;
        script.push_back(0x0C); // PUSHDATA1
        script.push_back(33);   // 33 bytes for compressed public key
        
        auto pub_key_bytes = public_key.ToArray();
        script.insert(script.end(), pub_key_bytes.begin(), pub_key_bytes.end());
        
        script.push_back(0x41); // SYSCALL
        script.push_back(0x9b); // System.Crypto.CheckWitness
        script.push_back(0xf5);
        script.push_back(0xbe);
        
        return script;
    }
    
    // Base64 encoding helper
    static std::string Base64Encode(const std::string& input) {
        // Simple Base64 implementation for testing
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        
        int val = 0, valb = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                result.push_back(chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) {
            result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
        }
        while (result.size() % 4) {
            result.push_back('=');
        }
        return result;
    }
};

} // namespace neo::tests