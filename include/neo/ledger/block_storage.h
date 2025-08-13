/**
 * @file block_storage.h
 * @brief Block structure and validation
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/uint256.h>
#include <neo/ledger/block.h>
#include <neo/persistence/data_cache.h>

#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace neo::ledger
{
/**
 * @brief Handles block storage and retrieval.
 */
class BlockStorage
{
   public:
    /**
     * @brief Constructs a BlockStorage.
     * @param dataCache The data cache.
     */
    explicit BlockStorage(std::shared_ptr<persistence::DataCache> dataCache);

    /**
     * @brief Gets a block by hash.
     * @param hash The hash of the block.
     * @return The block, or nullptr if not found.
     */
    std::shared_ptr<Block> GetBlock(const io::UInt256& hash) const;

    /**
     * @brief Gets a block by index.
     * @param index The index of the block.
     * @return The block, or nullptr if not found.
     */
    std::shared_ptr<Block> GetBlock(uint32_t index) const;

    /**
     * @brief Gets a block header by hash.
     * @param hash The hash of the block.
     * @return The block header, or nullptr if not found.
     */
    std::shared_ptr<BlockHeader> GetBlockHeader(const io::UInt256& hash) const;

    /**
     * @brief Gets a block header by index.
     * @param index The index of the block.
     * @return The block header, or nullptr if not found.
     */
    std::shared_ptr<BlockHeader> GetBlockHeader(uint32_t index) const;

    /**
     * @brief Adds a block to storage.
     * @param block The block to add.
     * @param snapshot The snapshot to use.
     * @return True if the block was added, false otherwise.
     */
    bool AddBlock(const Block& block, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Adds a block header to storage.
     * @param header The block header to add.
     * @param snapshot The snapshot to use.
     * @return True if the block header was added, false otherwise.
     */
    bool AddBlockHeader(const BlockHeader& header, std::shared_ptr<persistence::DataCache> snapshot);

    /**
     * @brief Checks if a block exists.
     * @param hash The hash of the block.
     * @return True if the block exists, false otherwise.
     */
    bool ContainsBlock(const io::UInt256& hash) const;

    /**
     * @brief Gets the hash of a block by index.
     * @param index The index of the block.
     * @return The hash of the block, or std::nullopt if not found.
     */
    std::optional<io::UInt256> GetBlockHash(uint32_t index) const;

    /**
     * @brief Gets the next block hash.
     * @param hash The hash of the current block.
     * @return The hash of the next block, or std::nullopt if not found.
     */
    std::optional<io::UInt256> GetNextBlockHash(const io::UInt256& hash) const;

   private:
    std::shared_ptr<persistence::DataCache> dataCache_;
    mutable std::mutex mutex_;
    mutable std::unordered_map<io::UInt256, std::shared_ptr<Block>> blocks_;
    mutable std::unordered_map<io::UInt256, std::shared_ptr<BlockHeader>> headers_;
    mutable std::unordered_map<uint32_t, io::UInt256> blockHashes_;
    mutable std::unordered_map<io::UInt256, io::UInt256> nextBlockHashes_;
};
}  // namespace neo::ledger
