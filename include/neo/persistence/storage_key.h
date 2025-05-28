#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>

namespace neo::persistence
{
    /**
     * @brief Represents a key in the storage.
     */
    class StorageKey : public io::ISerializable
    {
    public:
        /**
         * @brief Constructs an empty StorageKey.
         */
        StorageKey();

        /**
         * @brief Constructs a StorageKey with the specified script hash.
         * @param scriptHash The script hash.
         */
        explicit StorageKey(const io::UInt160& scriptHash);

        /**
         * @brief Constructs a StorageKey with the specified script hash and key.
         * @param scriptHash The script hash.
         * @param key The key.
         */
        StorageKey(const io::UInt160& scriptHash, const io::ByteVector& key);

        /**
         * @brief Constructs a StorageKey from a ByteVector.
         * @param data The serialized StorageKey data.
         */
        explicit StorageKey(const io::ByteVector& data);

        /**
         * @brief Gets the script hash.
         * @return The script hash.
         */
        const io::UInt160& GetScriptHash() const;

        /**
         * @brief Gets the key.
         * @return The key.
         */
        const io::ByteVector& GetKey() const;

        /**
         * @brief Serializes the StorageKey to a binary writer.
         * @param writer The binary writer.
         */
        void Serialize(io::BinaryWriter& writer) const override;

        /**
         * @brief Deserializes the StorageKey from a binary reader.
         * @param reader The binary reader.
         */
        void Deserialize(io::BinaryReader& reader) override;

        /**
         * @brief Checks if this StorageKey is equal to another StorageKey.
         * @param other The other StorageKey.
         * @return True if the StorageKeys are equal, false otherwise.
         */
        bool operator==(const StorageKey& other) const;

        /**
         * @brief Checks if this StorageKey is not equal to another StorageKey.
         * @param other The other StorageKey.
         * @return True if the StorageKeys are not equal, false otherwise.
         */
        bool operator!=(const StorageKey& other) const;

        /**
         * @brief Checks if this StorageKey is less than another StorageKey.
         * @param other The other StorageKey.
         * @return True if this StorageKey is less than the other StorageKey, false otherwise.
         */
        bool operator<(const StorageKey& other) const;

    private:
        io::UInt160 scriptHash_;
        io::ByteVector key_;
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
            // Combine the hash of the script hash and the key
            size_t h1 = hash<neo::io::UInt160>()(key.GetScriptHash());
            size_t h2 = hash<neo::io::ByteVector>()(key.GetKey());
            return h1 ^ (h2 << 1);
        }
    };
}
