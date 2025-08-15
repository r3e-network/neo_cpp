#pragma once

/**
 * @file converter.h
 * @brief Utility converters for Neo blockchain data types
 * @author Neo C++ Team
 * @date 2025
 */

#include <neo/sdk/core/types.h>
#include <string>
#include <vector>
#include <cstdint>

namespace neo::sdk::utils {

/**
 * @brief Hex string conversion utilities
 */
class HexConverter {
public:
    /**
     * @brief Convert bytes to hex string
     * @param bytes Input bytes
     * @param prefix Add "0x" prefix
     * @return Hex string
     */
    static std::string BytesToHex(const std::vector<uint8_t>& bytes, bool prefix = false);
    
    /**
     * @brief Convert hex string to bytes
     * @param hex Hex string (with or without "0x" prefix)
     * @return Byte vector
     */
    static std::vector<uint8_t> HexToBytes(const std::string& hex);
    
    /**
     * @brief Check if string is valid hex
     * @param hex String to check
     * @return true if valid hex
     */
    static bool IsValidHex(const std::string& hex);
    
    /**
     * @brief Reverse hex string (for little-endian conversion)
     * @param hex Input hex string
     * @return Reversed hex string
     */
    static std::string ReverseHex(const std::string& hex);
};

/**
 * @brief Number conversion utilities
 */
class NumberConverter {
public:
    /**
     * @brief Convert Fixed8 to decimal
     * @param fixed8 Fixed8 value (8 decimal places)
     * @return Decimal value
     */
    static double Fixed8ToDecimal(int64_t fixed8);
    
    /**
     * @brief Convert decimal to Fixed8
     * @param decimal Decimal value
     * @return Fixed8 value
     */
    static int64_t DecimalToFixed8(double decimal);
    
    /**
     * @brief Convert amount with decimals
     * @param amount Raw amount
     * @param decimals Number of decimals
     * @return Decimal amount
     */
    static double ToDecimal(uint64_t amount, uint8_t decimals);
    
    /**
     * @brief Convert from decimal amount
     * @param decimal Decimal amount
     * @param decimals Number of decimals
     * @return Raw amount
     */
    static uint64_t FromDecimal(double decimal, uint8_t decimals);
    
    /**
     * @brief Format amount for display
     * @param amount Raw amount
     * @param decimals Number of decimals
     * @param symbol Token symbol
     * @return Formatted string (e.g., "100.5 NEO")
     */
    static std::string FormatAmount(uint64_t amount, uint8_t decimals, const std::string& symbol = "");
};

/**
 * @brief Script conversion utilities
 */
class ScriptConverter {
public:
    /**
     * @brief Convert script to OpCode string
     * @param script Script bytes
     * @return Human-readable OpCode string
     */
    static std::string ScriptToOpCodeString(const std::vector<uint8_t>& script);
    
    /**
     * @brief Parse script hash from string
     * @param input Script hash string (with or without "0x")
     * @return Script hash
     */
    static core::UInt160 ParseScriptHash(const std::string& input);
    
    /**
     * @brief Build verification script for public key
     * @param publicKey Public key bytes
     * @return Verification script
     */
    static std::vector<uint8_t> PublicKeyToVerificationScript(const std::vector<uint8_t>& publicKey);
    
    /**
     * @brief Build multi-sig verification script
     * @param m Minimum signatures required
     * @param publicKeys Public keys
     * @return Multi-sig verification script
     */
    static std::vector<uint8_t> CreateMultiSigScript(uint8_t m, const std::vector<std::vector<uint8_t>>& publicKeys);
};

/**
 * @brief Time conversion utilities
 */
class TimeConverter {
public:
    /**
     * @brief Convert Unix timestamp to block time
     * @param timestamp Unix timestamp in seconds
     * @return Block timestamp (milliseconds)
     */
    static uint64_t UnixToBlockTime(uint64_t timestamp);
    
    /**
     * @brief Convert block time to Unix timestamp
     * @param blockTime Block timestamp (milliseconds)
     * @return Unix timestamp in seconds
     */
    static uint64_t BlockTimeToUnix(uint64_t blockTime);
    
    /**
     * @brief Format block time for display
     * @param blockTime Block timestamp
     * @return Formatted date/time string
     */
    static std::string FormatBlockTime(uint64_t blockTime);
    
    /**
     * @brief Calculate block generation time
     * @param blockIndex Block index
     * @param millisecondsPerBlock Milliseconds per block (15000 for Neo)
     * @return Estimated timestamp
     */
    static uint64_t EstimateBlockTime(uint32_t blockIndex, uint64_t millisecondsPerBlock = 15000);
};

/**
 * @brief JSON conversion utilities
 */
class JsonConverter {
public:
    /**
     * @brief Convert transaction to JSON
     * @param tx Transaction
     * @return JSON string
     */
    static std::string TransactionToJson(const core::Transaction& tx);
    
    /**
     * @brief Convert block to JSON
     * @param block Block
     * @return JSON string
     */
    static std::string BlockToJson(const core::Block& block);
    
    /**
     * @brief Parse transaction from JSON
     * @param json JSON string
     * @return Transaction
     */
    static core::Transaction TransactionFromJson(const std::string& json);
    
    /**
     * @brief Parse block from JSON
     * @param json JSON string
     * @return Block
     */
    static core::Block BlockFromJson(const std::string& json);
};

/**
 * @brief String utilities
 */
class StringUtils {
public:
    /**
     * @brief Trim whitespace from string
     * @param str Input string
     * @return Trimmed string
     */
    static std::string Trim(const std::string& str);
    
    /**
     * @brief Convert to lowercase
     * @param str Input string
     * @return Lowercase string
     */
    static std::string ToLower(const std::string& str);
    
    /**
     * @brief Convert to uppercase
     * @param str Input string
     * @return Uppercase string
     */
    static std::string ToUpper(const std::string& str);
    
    /**
     * @brief Split string by delimiter
     * @param str Input string
     * @param delimiter Delimiter character
     * @return Vector of substrings
     */
    static std::vector<std::string> Split(const std::string& str, char delimiter);
    
    /**
     * @brief Join strings with delimiter
     * @param strings Vector of strings
     * @param delimiter Delimiter string
     * @return Joined string
     */
    static std::string Join(const std::vector<std::string>& strings, const std::string& delimiter);
};

} // namespace neo::sdk::utils