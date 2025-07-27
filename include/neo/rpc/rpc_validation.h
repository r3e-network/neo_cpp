#pragma once

#include <neo/core/safe_conversions.h>
#include <neo/cryptography/hash.h>
#include <neo/io/json.h>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::rpc
{

/**
 * @brief RPC parameter validation utilities
 */
class RpcValidation
{
  public:
    /**
     * @brief Validate a hex string
     * @param hex The hex string to validate
     * @param expectedLength Expected byte length (0 for any length)
     * @return true if valid
     */
    static bool IsValidHexString(const std::string& hex, size_t expectedLength = 0)
    {
        if (hex.empty())
            return false;

        // Check if starts with 0x
        size_t start = 0;
        if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
        {
            start = 2;
        }

        // Must have even number of hex digits
        size_t hexLength = hex.length() - start;
        if (hexLength % 2 != 0)
            return false;

        // Check expected length
        if (expectedLength > 0 && hexLength / 2 != expectedLength)
        {
            return false;
        }

        // Validate hex characters
        for (size_t i = start; i < hex.length(); i++)
        {
            char c = hex[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Validate a block hash
     * @param hash The hash to validate
     * @throws std::invalid_argument if invalid
     */
    static void ValidateBlockHash(const std::string& hash)
    {
        if (!IsValidHexString(hash, 32))
        {
            throw std::invalid_argument("Invalid block hash format (expected 32 bytes hex)");
        }
    }

    /**
     * @brief Validate a transaction hash
     * @param hash The hash to validate
     * @throws std::invalid_argument if invalid
     */
    static void ValidateTransactionHash(const std::string& hash)
    {
        if (!IsValidHexString(hash, 32))
        {
            throw std::invalid_argument("Invalid transaction hash format (expected 32 bytes hex)");
        }
    }

    /**
     * @brief Validate a script hash
     * @param hash The hash to validate
     * @throws std::invalid_argument if invalid
     */
    static void ValidateScriptHash(const std::string& hash)
    {
        if (!IsValidHexString(hash, 20))
        {
            throw std::invalid_argument("Invalid script hash format (expected 20 bytes hex)");
        }
    }

    /**
     * @brief Validate a public key
     * @param pubkey The public key to validate
     * @throws std::invalid_argument if invalid
     */
    static void ValidatePublicKey(const std::string& pubkey)
    {
        if (!IsValidHexString(pubkey, 33) && !IsValidHexString(pubkey, 65))
        {
            throw std::invalid_argument("Invalid public key format (expected 33 or 65 bytes hex)");
        }
    }

    /**
     * @brief Validate a block index
     * @param index The index to validate
     * @throws std::invalid_argument if invalid
     */
    static void ValidateBlockIndex(uint32_t index)
    {
        // Block index is always valid as uint32_t
    }

    /**
     * @brief Validate a Neo address
     * @param address The address to validate
     * @throws std::invalid_argument if invalid
     */
    static void ValidateAddress(const std::string& address)
    {
        if (address.empty())
        {
            throw std::invalid_argument("Address cannot be empty");
        }

        // Neo addresses start with 'N' for mainnet, 'A' for testnet
        if (address[0] != 'N' && address[0] != 'A')
        {
            throw std::invalid_argument("Invalid address format (must start with N or A)");
        }

        // Length check (typically 34 characters)
        if (address.length() < 25 || address.length() > 34)
        {
            throw std::invalid_argument("Invalid address length");
        }

        // Base58 character check
        static const std::string base58Chars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
        for (char c : address)
        {
            if (base58Chars.find(c) == std::string::npos)
            {
                throw std::invalid_argument("Invalid character in address: " + std::string(1, c));
            }
        }
    }

    /**
     * @brief Validate script for invocation
     * @param script The script hex string
     * @throws std::invalid_argument if invalid
     */
    static void ValidateScript(const std::string& script)
    {
        if (!IsValidHexString(script))
        {
            throw std::invalid_argument("Invalid script format (expected hex string)");
        }

        // Maximum script size check (1MB)
        if (script.length() / 2 > 1024 * 1024)
        {
            throw std::invalid_argument("Script too large (maximum 1MB)");
        }
    }

    /**
     * @brief Validate JSON parameters array
     * @param params The parameters to validate
     * @param minCount Minimum number of parameters
     * @param maxCount Maximum number of parameters
     * @throws std::invalid_argument if invalid
     */
    static void ValidateParamCount(const io::json::JArray& params, size_t minCount, size_t maxCount)
    {
        size_t count = params.size();
        if (count < minCount)
        {
            throw std::invalid_argument("Too few parameters (expected at least " + std::to_string(minCount) + ", got " +
                                        std::to_string(count) + ")");
        }
        if (count > maxCount)
        {
            throw std::invalid_argument("Too many parameters (expected at most " + std::to_string(maxCount) + ", got " +
                                        std::to_string(count) + ")");
        }
    }

    /**
     * @brief Extract and validate string parameter
     * @param params Parameter array
     * @param index Parameter index
     * @param paramName Parameter name for error messages
     * @return The string value
     * @throws std::invalid_argument if invalid
     */
    static std::string GetStringParam(const io::json::JArray& params, size_t index, const std::string& paramName)
    {
        if (index >= params.size())
        {
            throw std::invalid_argument("Missing required parameter: " + paramName);
        }

        auto param = params[index];
        if (param->GetType() != io::json::JType::String)
        {
            throw std::invalid_argument(paramName + " must be a string");
        }

        return std::static_pointer_cast<io::json::JString>(param)->GetValue();
    }

    /**
     * @brief Extract and validate integer parameter
     * @param params Parameter array
     * @param index Parameter index
     * @param paramName Parameter name for error messages
     * @return The integer value
     * @throws std::invalid_argument if invalid
     */
    static int64_t GetIntParam(const io::json::JArray& params, size_t index, const std::string& paramName)
    {
        if (index >= params.size())
        {
            throw std::invalid_argument("Missing required parameter: " + paramName);
        }

        auto param = params[index];
        if (param->GetType() != io::json::JType::Number)
        {
            throw std::invalid_argument(paramName + " must be a number");
        }

        return std::static_pointer_cast<io::json::JNumber>(param)->GetInt64();
    }

    /**
     * @brief Extract and validate boolean parameter
     * @param params Parameter array
     * @param index Parameter index
     * @param paramName Parameter name for error messages
     * @return The boolean value
     * @throws std::invalid_argument if invalid
     */
    static bool GetBoolParam(const io::json::JArray& params, size_t index, const std::string& paramName)
    {
        if (index >= params.size())
        {
            throw std::invalid_argument("Missing required parameter: " + paramName);
        }

        auto param = params[index];
        if (param->GetType() != io::json::JType::Boolean)
        {
            throw std::invalid_argument(paramName + " must be a boolean");
        }

        return std::static_pointer_cast<io::json::JBoolean>(param)->GetValue();
    }

    /**
     * @brief Extract and validate optional string parameter
     * @param params Parameter array
     * @param index Parameter index
     * @param defaultValue Default value if not present
     * @return The string value or default
     */
    static std::string GetOptionalStringParam(const io::json::JArray& params, size_t index,
                                              const std::string& defaultValue = "")
    {
        if (index >= params.size())
        {
            return defaultValue;
        }

        auto param = params[index];
        if (param->GetType() == io::json::JType::Null)
        {
            return defaultValue;
        }

        if (param->GetType() != io::json::JType::String)
        {
            throw std::invalid_argument("Parameter at index " + std::to_string(index) + " must be a string");
        }

        return std::static_pointer_cast<io::json::JString>(param)->GetValue();
    }

    /**
     * @brief Validate gas amount
     * @param gas Gas amount as string
     * @throws std::invalid_argument if invalid
     */
    static void ValidateGasAmount(const std::string& gas)
    {
        try
        {
            double amount = core::SafeConversions::SafeToDouble(gas);
            if (amount < 0)
            {
                throw std::invalid_argument("Gas amount cannot be negative");
            }
            if (amount > 10000000000)
            {  // 10 billion GAS max
                throw std::invalid_argument("Gas amount too large");
            }
        }
        catch (const std::runtime_error& e)
        {
            throw std::invalid_argument("Invalid gas amount: " + std::string(e.what()));
        }
    }
};

}  // namespace neo::rpc