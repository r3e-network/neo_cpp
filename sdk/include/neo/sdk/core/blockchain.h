#pragma once

#include <neo/sdk/core/types.h>
#include <memory>

namespace neo::sdk::core {

/**
 * @brief High-level blockchain interface for querying blockchain data
 */
class Blockchain {
public:
    /**
     * @brief Get a block by its hash
     * @param hash The block hash
     * @return Shared pointer to the block, or nullptr if not found
     */
    static std::shared_ptr<Block> GetBlock(const UInt256& hash);
    
    /**
     * @brief Get a block by its height
     * @param height The block height
     * @return Shared pointer to the block, or nullptr if not found
     */
    static std::shared_ptr<Block> GetBlock(uint32_t height);
    
    /**
     * @brief Get a transaction by its hash
     * @param hash The transaction hash
     * @return Shared pointer to the transaction, or nullptr if not found
     */
    static std::shared_ptr<Transaction> GetTransaction(const UInt256& hash);
    
    /**
     * @brief Get the current blockchain height
     * @return The current height
     */
    static uint32_t GetCurrentHeight();
    
    /**
     * @brief Get a block header by height
     * @param height The block height
     * @return Shared pointer to the header, or nullptr if not found
     */
    static std::shared_ptr<Header> GetHeader(uint32_t height);
    
    /**
     * @brief Get the best block hash
     * @return The hash of the best (latest) block
     */
    static UInt256 GetBestBlockHash();
    
    /**
     * @brief Verify if a block exists
     * @param hash The block hash
     * @return true if the block exists
     */
    static bool ContainsBlock(const UInt256& hash);
    
    /**
     * @brief Verify if a transaction exists
     * @param hash The transaction hash
     * @return true if the transaction exists
     */
    static bool ContainsTransaction(const UInt256& hash);
    
    /**
     * @brief Get multiple blocks in a range
     * @param start Start height (inclusive)
     * @param count Number of blocks to retrieve
     * @return Vector of blocks
     */
    static std::vector<std::shared_ptr<Block>> GetBlocks(uint32_t start, uint32_t count);
    
    /**
     * @brief Get the genesis block
     * @return The genesis block
     */
    static std::shared_ptr<Block> GetGenesisBlock();

private:
    // Private constructor - static class only
    Blockchain() = delete;
};

} // namespace neo::sdk::core