/**
 * @file hash_index_state.h
 * @brief Hashing algorithms
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/uint256.h>
#include <neo/vm/stack_item.h>

#include <memory>

namespace neo::smartcontract::native
{
/**
 * @brief Represents a hash and index state.
 */
class HashIndexState
{
   public:
    /**
     * @brief Constructs a HashIndexState.
     */
    HashIndexState();

    /**
     * @brief Constructs a HashIndexState with the specified hash and index.
     * @param hash The hash.
     * @param index The index.
     */
    HashIndexState(const io::UInt256& hash, uint32_t index);

    /**
     * @brief Gets the hash.
     * @return The hash.
     */
    const io::UInt256& GetHash() const;

    /**
     * @brief Sets the hash.
     * @param hash The hash.
     */
    void SetHash(const io::UInt256& hash);

    /**
     * @brief Gets the index.
     * @return The index.
     */
    uint32_t GetIndex() const;

    /**
     * @brief Sets the index.
     * @param index The index.
     */
    void SetIndex(uint32_t index);

    /**
     * @brief Deserializes the hash index state from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader);

    /**
     * @brief Serializes the hash index state to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const;

    /**
     * @brief Converts the hash index state to a stack item.
     * @return The stack item.
     */
    std::shared_ptr<vm::StackItem> ToStackItem() const;

    /**
     * @brief Converts a stack item to a hash index state.
     * @param item The stack item.
     */
    void FromStackItem(const std::shared_ptr<vm::StackItem>& item);

   private:
    io::UInt256 hash_;
    uint32_t index_;
};
}  // namespace neo::smartcontract::native
