#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <memory>

namespace neo::smartcontract
{
    /**
     * @brief Storage key for smart contract storage
     */
    class StorageKey : public io::ISerializable
    {
    private:
        io::UInt160 script_hash_;
        io::ByteVector key_;
        
    public:
        /**
         * @brief Default constructor
         */
        StorageKey() = default;
        
        /**
         * @brief Constructor with script hash and key
         * @param script_hash Contract script hash
         * @param key Storage key bytes
         */
        StorageKey(const io::UInt160& script_hash, const io::ByteVector& key);
        
        /**
         * @brief Get script hash
         * @return Script hash
         */
        const io::UInt160& GetScriptHash() const { return script_hash_; }
        
        /**
         * @brief Set script hash
         * @param script_hash Script hash
         */
        void SetScriptHash(const io::UInt160& script_hash) { script_hash_ = script_hash; }
        
        /**
         * @brief Get key bytes
         * @return Key bytes
         */
        const io::ByteVector& GetKey() const { return key_; }
        
        /**
         * @brief Set key bytes
         * @param key Key bytes
         */
        void SetKey(const io::ByteVector& key) { key_ = key; }
        
        /**
         * @brief Get size in bytes
         * @return Size in bytes
         */
        size_t GetSize() const override;
        
        // ISerializable implementation
        void Serialize(io::BinaryWriter& writer) const override;
        void Deserialize(io::BinaryReader& reader) override;
        
        // Comparison operators
        bool operator==(const StorageKey& other) const;
        bool operator!=(const StorageKey& other) const { return !(*this == other); }
        bool operator<(const StorageKey& other) const;
        
        /**
         * @brief Convert to string representation
         * @return String representation
         */
        std::string ToString() const;
        
        /**
         * @brief Get hash code for use in hash maps
         * @return Hash code
         */
        size_t GetHashCode() const;
    };
}

// Hash function for std::unordered_map
namespace std
{
    template<>
    struct hash<neo::smartcontract::StorageKey>
    {
        size_t operator()(const neo::smartcontract::StorageKey& key) const
        {
            return key.GetHashCode();
        }
    };
}