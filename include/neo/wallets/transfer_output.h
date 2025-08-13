/**
 * @file transfer_output.h
 * @brief Transfer Output
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <cstdint>
#include <string>

namespace neo::wallets
{
/**
 * @brief Represents an output for transferring assets.
 */
class TransferOutput
{
   public:
    /**
     * @brief Default constructor.
     */
    TransferOutput();

    /**
     * @brief Constructor with parameters.
     * @param asset_id The asset ID.
     * @param script_hash The recipient script hash.
     * @param amount The amount to transfer.
     * @param data Optional data.
     */
    TransferOutput(const io::UInt160& asset_id, const io::UInt160& script_hash, int64_t amount,
                   const std::string& data = "");

    /**
     * @brief Destructor.
     */
    ~TransferOutput() = default;

    /**
     * @brief Copy constructor.
     * @param other The other transfer output.
     */
    TransferOutput(const TransferOutput& other) = default;

    /**
     * @brief Move constructor.
     * @param other The other transfer output.
     */
    TransferOutput(TransferOutput&& other) noexcept = default;

    /**
     * @brief Copy assignment operator.
     * @param other The other transfer output.
     * @return Reference to this transfer output.
     */
    TransferOutput& operator=(const TransferOutput& other) = default;

    /**
     * @brief Move assignment operator.
     * @param other The other transfer output.
     * @return Reference to this transfer output.
     */
    TransferOutput& operator=(TransferOutput&& other) noexcept = default;

    /**
     * @brief Gets the asset ID.
     * @return The asset ID.
     */
    const io::UInt160& GetAssetId() const;

    /**
     * @brief Sets the asset ID.
     * @param asset_id The asset ID.
     */
    void SetAssetId(const io::UInt160& asset_id);

    /**
     * @brief Gets the recipient script hash.
     * @return The script hash.
     */
    const io::UInt160& GetScriptHash() const;

    /**
     * @brief Sets the recipient script hash.
     * @param script_hash The script hash.
     */
    void SetScriptHash(const io::UInt160& script_hash);

    /**
     * @brief Gets the transfer amount.
     * @return The amount.
     */
    int64_t GetAmount() const;

    /**
     * @brief Sets the transfer amount.
     * @param amount The amount.
     */
    void SetAmount(int64_t amount);

    /**
     * @brief Gets the optional data.
     * @return The data string.
     */
    const std::string& GetData() const;

    /**
     * @brief Sets the optional data.
     * @param data The data string.
     */
    void SetData(const std::string& data);

    /**
     * @brief Checks if this transfer output is valid.
     * @return True if valid, false otherwise.
     */
    bool IsValid() const;

    /**
     * @brief Equality operator.
     * @param other The other transfer output.
     * @return True if equal, false otherwise.
     */
    bool operator==(const TransferOutput& other) const;

    /**
     * @brief Inequality operator.
     * @param other The other transfer output.
     * @return True if not equal, false otherwise.
     */
    bool operator!=(const TransferOutput& other) const;

    /**
     * @brief Converts to string representation.
     * @return String representation.
     */
    std::string ToString() const;

   private:
    io::UInt160 asset_id_;
    io::UInt160 script_hash_;
    int64_t amount_;
    std::string data_;
};
}  // namespace neo::wallets
