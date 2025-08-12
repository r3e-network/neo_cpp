#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/uint256.h>

#include <cstdint>
#include <vector>

namespace neo::ledger
{
/**
 * @brief State root for MPT (Merkle Patricia Trie) state tracking
 *
 * This class represents a state root in the Neo blockchain, which is used
 * to track the state of the blockchain at a specific height. It follows
 * the C# Neo implementation for compatibility.
 */
class StateRoot
{
   private:
    uint8_t version_;
    uint32_t index_;                                     // Block index/height
    io::UInt256 root_hash_;                              // MPT root hash
    std::vector<cryptography::ecc::ECPoint> witnesses_;  // Witness signatures

   public:
    /**
     * @brief Default constructor
     */
    StateRoot() : version_(0), index_(0) {}

    /**
     * @brief Constructor with parameters
     * @param version State root version
     * @param index Block index
     * @param root_hash MPT root hash
     */
    StateRoot(uint8_t version, uint32_t index, const io::UInt256& root_hash)
        : version_(version), index_(index), root_hash_(root_hash)
    {
    }

    /**
     * @brief Get state root version
     * @return Version number
     */
    uint8_t GetVersion() const { return version_; }

    /**
     * @brief Set state root version
     * @param version Version number
     */
    void SetVersion(uint8_t version) { version_ = version; }

    /**
     * @brief Get block index
     * @return Block index/height
     */
    uint32_t GetIndex() const { return index_; }

    /**
     * @brief Set block index
     * @param index Block index/height
     */
    void SetIndex(uint32_t index) { index_ = index; }

    /**
     * @brief Get MPT root hash
     * @return Root hash
     */
    const io::UInt256& GetRoot() const { return root_hash_; }

    /**
     * @brief Set MPT root hash
     * @param root Root hash
     */
    void SetRoot(const io::UInt256& root) { root_hash_ = root; }

    /**
     * @brief Get witness signatures
     * @return Vector of witness signatures
     */
    const std::vector<cryptography::ecc::ECPoint>& GetWitnesses() const { return witnesses_; }

    /**
     * @brief Add witness signature
     * @param witness Witness signature to add
     */
    void AddWitness(const cryptography::ecc::ECPoint& witness) { witnesses_.push_back(witness); }

    /**
     * @brief Clear all witnesses
     */
    void ClearWitnesses() { witnesses_.clear(); }

    /**
     * @brief Calculate hash of this state root
     * @return Hash value
     */
    io::UInt256 GetHash() const;

    /**
     * @brief Verify witness signatures
     * @return True if all witnesses are valid
     */
    bool Verify() const;

    /**
     * @brief Serialize to byte array
     * @return Serialized bytes
     */
    std::vector<uint8_t> ToByteArray() const;

    /**
     * @brief Deserialize from byte array
     * @param data Byte array to deserialize
     * @return True if successful
     */
    bool FromByteArray(const std::vector<uint8_t>& data);

    /**
     * @brief Check if state root is valid
     * @return True if valid
     */
    bool IsValid() const { return index_ > 0 && !root_hash_.IsZero(); }
};

}  // namespace neo::ledger