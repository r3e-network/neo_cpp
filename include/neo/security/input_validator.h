#pragma once

#include <string>
#include <vector>
#include <regex>
#include <limits>
#include <algorithm>

namespace neo {
namespace security {

/**
 * @brief Input validation utility for Neo blockchain
 */
class InputValidator {
public:
    /**
     * @brief Validate a Neo address
     * @param address The address to validate
     * @return true if valid, false otherwise
     */
    static bool ValidateAddress(const std::string& address) {
        // Neo address is Base58 encoded and 34 characters long
        if (address.length() != 34) return false;
        if (address[0] != 'A' && address[0] != 'N') return false;
        
        // Check Base58 characters
        static const std::regex base58Pattern("^[123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz]+$");
        return std::regex_match(address, base58Pattern);
    }
    
    /**
     * @brief Validate a transaction hash
     * @param hash The hash to validate
     * @return true if valid, false otherwise
     */
    static bool ValidateTransactionHash(const std::string& hash) {
        // Transaction hash should be 66 characters (0x + 64 hex chars)
        if (hash.length() != 66) return false;
        if (hash.substr(0, 2) != "0x") return false;
        
        // Check hex characters
        static const std::regex hexPattern("^0x[0-9a-fA-F]{64}$");
        return std::regex_match(hash, hexPattern);
    }
    
    /**
     * @brief Validate a script hash
     * @param scriptHash The script hash to validate
     * @return true if valid, false otherwise
     */
    static bool ValidateScriptHash(const std::string& scriptHash) {
        // Script hash should be 42 characters (0x + 40 hex chars)
        if (scriptHash.length() != 42) return false;
        if (scriptHash.substr(0, 2) != "0x") return false;
        
        static const std::regex hexPattern("^0x[0-9a-fA-F]{40}$");
        return std::regex_match(scriptHash, hexPattern);
    }
    
    /**
     * @brief Validate an amount
     * @param amount The amount to validate
     * @return true if valid, false otherwise
     */
    static bool ValidateAmount(const std::string& amount) {
        try {
            double value = std::stod(amount);
            return value >= 0 && value <= 100000000000;  // Max 100 billion
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Sanitize user input to prevent injection attacks
     * @param input The input to sanitize
     * @return Sanitized string
     */
    static std::string SanitizeInput(const std::string& input) {
        std::string result;
        for (char c : input) {
            // Remove control characters and non-printable characters
            if (c >= 32 && c <= 126) {
                // Escape special characters
                if (c == '<' || c == '>' || c == '&' || c == '"' || c == '\'') {
                    result += '\\';
                }
                result += c;
            }
        }
        return result;
    }
    
    /**
     * @brief Validate a public key
     * @param publicKey The public key to validate
     * @return true if valid, false otherwise
     */
    static bool ValidatePublicKey(const std::string& publicKey) {
        // Compressed public key: 66 chars (33 bytes in hex)
        // Uncompressed public key: 130 chars (65 bytes in hex)
        if (publicKey.length() != 66 && publicKey.length() != 130) return false;
        
        static const std::regex hexPattern("^[0-9a-fA-F]+$");
        return std::regex_match(publicKey, hexPattern);
    }
    
    /**
     * @brief Check for SQL injection patterns
     * @param input The input to check
     * @return true if suspicious patterns found
     */
    static bool ContainsSQLInjection(const std::string& input) {
        static const std::vector<std::string> patterns = {
            "DROP TABLE", "DELETE FROM", "INSERT INTO", "UPDATE SET",
            "SELECT * FROM", "UNION SELECT", "--", "/*", "*/", "xp_", "sp_"
        };
        
        std::string upperInput = input;
        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
        
        for (const auto& pattern : patterns) {
            if (upperInput.find(pattern) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief Validate block height
     * @param height The block height to validate
     * @return true if valid, false otherwise
     */
    static bool ValidateBlockHeight(uint32_t height) {
        // Block height should be reasonable
        return height <= 100000000;  // Max 100 million blocks
    }
    
    /**
     * @brief Validate array size to prevent resource exhaustion
     * @param size The size to validate
     * @param maxSize Maximum allowed size
     * @return true if valid, false otherwise
     */
    static bool ValidateArraySize(size_t size, size_t maxSize = 10000) {
        return size <= maxSize;
    }
    
    /**
     * @brief Sanitize string for safe display
     * @param input The input string
     * @return Sanitized string
     */
    static std::string SanitizeString(const std::string& input) {
        return SanitizeInput(input);
    }
    
    /**
     * @brief Sanitize HTML content
     * @param input The HTML input
     * @return Sanitized HTML
     */
    static std::string SanitizeHTML(const std::string& input) {
        std::string result;
        for (char c : input) {
            switch (c) {
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '&': result += "&amp;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#x27;"; break;
                default: result += c;
            }
        }
        return result;
    }
    
    /**
     * @brief Sanitize SQL input
     * @param input The SQL input
     * @return Sanitized SQL string
     */
    static std::string SanitizeSQL(const std::string& input) {
        std::string result;
        for (char c : input) {
            if (c == '\'') {
                result += "''";  // Escape single quotes
            } else {
                result += c;
            }
        }
        return result;
    }
    
    /**
     * @brief Validate RPC method name
     * @param method The RPC method name
     * @return true if valid, false otherwise
     */
    static bool ValidateRPCMethod(const std::string& method) {
        // RPC method should only contain alphanumeric and underscore
        if (method.empty() || method.length() > 100) return false;
        
        static const std::regex methodPattern("^[a-zA-Z][a-zA-Z0-9_]*$");
        return std::regex_match(method, methodPattern);
    }
    
    /**
     * @brief Validate file system path
     * @param path The path to validate
     * @return true if valid, false otherwise
     */
    static bool ValidatePath(const std::string& path) {
        // Prevent path traversal attacks
        if (path.empty()) return false;
        if (path.find("..") != std::string::npos) return false;
        if (path.find("~") != std::string::npos) return false;
        if (path.find('\0') != std::string::npos) return false;
        
        // Check for invalid characters
        static const std::string invalidChars = "<>:|?*\"";
        for (char c : invalidChars) {
            if (path.find(c) != std::string::npos) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * @brief Check for general injection patterns
     * @param input The input to check
     * @return true if injection pattern found, false otherwise
     */
    static bool ContainsInjectionPattern(const std::string& input) {
        // Check for common injection patterns
        static const std::vector<std::string> patterns = {
            "<script", "</script>", "javascript:", "onload=", "onerror=",
            "DROP TABLE", "DELETE FROM", "INSERT INTO", "UPDATE SET",
            "UNION SELECT", "--", "/*", "*/", "xp_", "sp_",
            "../", "..\\", "%00", "\x00"
        };
        
        std::string upperInput = input;
        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
        
        for (const auto& pattern : patterns) {
            std::string upperPattern = pattern;
            std::transform(upperPattern.begin(), upperPattern.end(), upperPattern.begin(), ::toupper);
            if (upperInput.find(upperPattern) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
};

} // namespace security
} // namespace neo