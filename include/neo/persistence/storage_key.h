/**
 * @file storage_key.h
 * @brief Persistent storage management
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <cstdint>
#include <optional>
#include <shared_mutex>
#include <span>

namespace neo::persistence
{
// Forward declaration
class DataCache;

/**
 * @brief Represents a key in the storage for Neo N3.
 *
 * In Neo N3, storage keys consist of:
 * - Contract ID (4 bytes, little-endian)
 * - Key data (variable length)
 *
 * This replaces the old script hash + key format from Neo 2.x.
 */
class StorageKey : public io::ISerializable
{
   public:
    /**
     * @brief The length of the prefix (contract ID + prefix byte).
     */
    static constexpr size_t PREFIX_LENGTH = sizeof(int32_t) + sizeof(uint8_t);

    /**
     * @brief Constructs an empty StorageKey.
     */
    StorageKey();

    /**
     * @brief Constructs a StorageKey with the specified contract ID.
     * @param contractId The contract ID.
     */
    explicit StorageKey(int32_t contractId);

    /**
     * @brief Constructs a StorageKey with the specified contract ID and key.
     * @param contractId The contract ID.
     * @param key The key.
     */
    StorageKey(int32_t contractId, const io::ByteVector& key);

    /**
     * @brief Constructs a StorageKey from a ByteVector.
     * @param data The serialized StorageKey data.
     */
    explicit StorageKey(const io::ByteVector& data);

    /**
     * @brief Constructs a StorageKey from a UInt160 script hash (Neo 2.x compatibility).
     * @param scriptHash The script hash.
     */
    explicit StorageKey(const io::UInt160& scriptHash);

    /**
     * @brief Constructs a StorageKey from a UInt160 script hash and key (Neo 2.x compatibility).
     * @param scriptHash The script hash.
     * @param key The key.
     */
    StorageKey(const io::UInt160& scriptHash, const io::ByteVector& key);

    /**
     * @brief Gets the contract ID.
     * @return The contract ID.
     */
    int32_t GetId() const { return id_; }

    /**
     * @brief Gets the key.
     * @return The key.
     */
    const io::ByteVector& GetKey() const;

    /**
     * @brief Gets the total length of the storage key.
     * @return The length.
     */
    size_t GetLength() const;

    /**
     * @brief Creates a storage key with contract ID and prefix.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix);

    /**
     * @brief Creates a storage key with contract ID, prefix, and byte content.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param content The content byte.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, uint8_t content);

    /**
     * @brief Creates a storage key with contract ID, prefix, and UInt160.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param hash The UInt160 hash.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, const io::UInt160& hash);

    /**
     * @brief Creates a storage key with contract ID, prefix, and UInt256.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param hash The UInt256 hash.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, const io::UInt256& hash);

    /**
     * @brief Creates a storage key with contract ID, prefix, and ECPoint.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param publicKey The public key.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, const cryptography::ecc::ECPoint& publicKey);

    /**
     * @brief Creates a storage key with contract ID, prefix, and int32 (big-endian).
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param bigEndian The big-endian integer.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, int32_t bigEndian);

    /**
     * @brief Creates a storage key with contract ID, prefix, and uint32 (big-endian).
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param bigEndian The big-endian unsigned integer.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, uint32_t bigEndian);

    /**
     * @brief Creates a storage key with contract ID, prefix, and int64 (big-endian).
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param bigEndian The big-endian long integer.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, int64_t bigEndian);

    /**
     * @brief Creates a storage key with contract ID, prefix, and uint64 (big-endian).
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param bigEndian The big-endian unsigned long integer.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, uint64_t bigEndian);

    /**
     * @brief Creates a storage key with contract ID, prefix, and byte span.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param content The content bytes.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, const std::span<const uint8_t>& content);

    /**
     * @brief Creates a storage key with contract ID, prefix, UInt256, and UInt160.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     * @param hash The UInt256 hash.
     * @param signer The UInt160 signer.
     * @return The storage key.
     */
    static StorageKey Create(int32_t id, uint8_t prefix, const io::UInt256& hash, const io::UInt160& signer);

    /**
     * @brief Creates a search prefix for finding storage keys.
     * @param id The contract ID.
     * @param prefix The prefix bytes.
     * @return The search prefix.
     */
    static io::ByteVector CreateSearchPrefix(int32_t id, const std::span<const uint8_t>& prefix);

    /**
     * @brief Converts the storage key to a byte array.
     * @return The byte array.
     */
    io::ByteVector ToArray() const;

    /**
     * @brief Gets the script hash for the contract ID.
     * @return The script hash (UInt160).
     * @note This implementation requires blockchain state access for full functionality.
     */
    io::UInt160 GetScriptHash() const;

    /**
     * @brief Creates a storage key with script hash and prefix.
     * @param scriptHash The script hash.
     * @param prefix The prefix byte.
     * @return The storage key.
     */
    static StorageKey Create(const io::UInt160& scriptHash, uint8_t prefix);

    /**
     * @brief Creates a storage key with contract lookup via DataCache.
     * @param dataCache The data cache for contract lookup.
     * @param scriptHash The script hash.
     * @param prefix The prefix byte.
     * @return The storage key.
     */
    static StorageKey CreateWithContract(const DataCache& dataCache, const io::UInt160& scriptHash, uint8_t prefix);

    /**
     * @brief Resolves the contract ID from script hash using DataCache.
     * @param dataCache The data cache for contract lookup.
     * @return The resolved contract ID.
     */
    int32_t ResolveContractId(const DataCache& dataCache) const;

    /**
     * @brief Gets the contract ID.
     * @return The contract ID.
     * @throws std::runtime_error if contract ID requires resolution.
     */
    int32_t GetContractId() const;

    /**
     * @brief Deserializes from a byte array.
     * @param data The byte array.
     */
    void DeserializeFromArray(const std::span<const uint8_t>& data);

    // ISerializable implementation
    /**
     * @brief Serializes the storage key to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the storage key from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    // Comparison operators
    bool operator==(const StorageKey& other) const;
    bool operator!=(const StorageKey& other) const;
    bool operator<(const StorageKey& other) const;

    /**
     * @brief Checks if this storage key equals another (C# compatibility).
     * @param other The other storage key.
     * @return True if equal, false otherwise.
     */
    bool Equals(const StorageKey& other) const;

    /**
     * @brief Compares this storage key with another (C# compatibility).
     * @param other The other storage key.
     * @return -1 if less, 0 if equal, 1 if greater.
     */
    int CompareTo(const StorageKey& other) const;

   private:
    mutable int32_t id_ = 0;
    io::ByteVector key_;
    mutable std::optional<io::UInt160> scriptHash_;  // For Neo 2.x compatibility
    mutable io::ByteVector cache_;                   // Cached serialized form
    mutable bool cacheValid_ = false;
    mutable bool requiresLookup_ = false;  // True if contract ID needs resolution

    /**
     * @brief Fills the header (contract ID + prefix) into a span.
     * @param data The span to fill.
     * @param id The contract ID.
     * @param prefix The prefix byte.
     */
    static void FillHeader(std::span<uint8_t> data, int32_t id, uint8_t prefix);

    /**
     * @brief Builds the cached serialized form.
     * @return The serialized data.
     */
    io::ByteVector Build() const;
};
}  // namespace neo::persistence

// Add hash function for StorageKey to be used with std::unordered_map
namespace std
{
template <>
struct hash<neo::persistence::StorageKey>
{
    size_t operator()(const neo::persistence::StorageKey& key) const noexcept
    {
        // Combine the hash of the contract ID and the key
        size_t h1 = hash<int32_t>()(key.GetId());
        size_t h2 = hash<neo::io::ByteVector>()(key.GetKey());
        return h1 ^ (h2 << 1);
    }
};
}  // namespace std