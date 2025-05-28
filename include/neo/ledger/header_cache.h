#pragma once

#include <neo/ledger/block_header.h>
#include <neo/io/uint256.h>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace neo::ledger
{
    /**
     * @brief Cache for block headers to improve performance.
     */
    class HeaderCache
    {
    public:
        /**
         * @brief Constructor.
         * @param max_size Maximum number of headers to cache.
         */
        explicit HeaderCache(size_t max_size = 10000);

        /**
         * @brief Destructor.
         */
        ~HeaderCache() = default;

        /**
         * @brief Adds a header to the cache.
         * @param header The header to add.
         */
        void Add(std::shared_ptr<BlockHeader> header);

        /**
         * @brief Gets a header from the cache.
         * @param hash The header hash.
         * @return The header if found, nullptr otherwise.
         */
        std::shared_ptr<BlockHeader> Get(const io::UInt256& hash) const;

        /**
         * @brief Checks if a header exists in the cache.
         * @param hash The header hash.
         * @return True if exists, false otherwise.
         */
        bool Contains(const io::UInt256& hash) const;

        /**
         * @brief Removes a header from the cache.
         * @param hash The header hash.
         * @return True if removed, false if not found.
         */
        bool Remove(const io::UInt256& hash);

        /**
         * @brief Clears all headers from the cache.
         */
        void Clear();

        /**
         * @brief Gets the number of headers in the cache.
         * @return The number of headers.
         */
        size_t Size() const;

        /**
         * @brief Gets the maximum cache size.
         * @return The maximum size.
         */
        size_t MaxSize() const;

        /**
         * @brief Checks if the cache is full.
         * @return True if full, false otherwise.
         */
        bool IsFull() const;

    private:
        mutable std::mutex mutex_;
        std::unordered_map<io::UInt256, std::shared_ptr<BlockHeader>> headers_;
        size_t max_size_;

        /**
         * @brief Evicts the oldest header if cache is full.
         */
        void EvictIfNeeded();
    };
}
