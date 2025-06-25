#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <cstdint>
#include <span>

namespace neo::persistence
{
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
         * @brief Gets the contract ID.
         * @return The contract ID.
         */
        int32_t GetId() const { return id_; }

        /**
         * @brief Gets the key.
         * @return The key.
         */
        const io::ByteVector& GetKey() const { return key_; }

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
         * @note This is a placeholder implementation that needs blockchain state access.
         */
        io::UInt160 GetScriptHash() const;

        // ISerializable implementation
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;

        // Comparison operators
        bool operator==(const StorageKey& other) const;
        bool operator!=(const StorageKey& other) const;
        bool operator<(const StorageKey& other) const;

    private:
        int32_t id_ = 0;
        io::ByteVector key_;
        mutable io::ByteVector cache_;  // Cached serialized form
        mutable bool cacheValid_ = false;

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
}

// Add hash function for StorageKey to be used with std::unordered_map
namespace std
{
    template<>
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
}