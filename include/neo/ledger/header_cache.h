#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <neo/io/uint256.h>
#include <neo/ledger/header.h>
#include <shared_mutex>
#include <unordered_map>

namespace neo::ledger
{
/**
 * @brief Header cache for efficient blockchain header synchronization.
 */
class HeaderCache
{
  public:
    /**
     * @brief Constructs a header cache.
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
     * @return True if added successfully, false otherwise.
     */
    bool Add(std::shared_ptr<Header> header);

    /**
     * @brief Gets a header by hash.
     * @param hash The hash of the header.
     * @return The header, or nullptr if not found.
     */
    std::shared_ptr<Header> Get(const io::UInt256& hash) const;

    /**
     * @brief Gets a header by index.
     * @param index The index of the header.
     * @return The header, or nullptr if not found.
     */
    std::shared_ptr<Header> Get(uint32_t index) const;

    /**
     * @brief Gets the last header in the cache.
     * @return The last header, or nullptr if cache is empty.
     */
    std::shared_ptr<Header> GetLast() const;

    /**
     * @brief Checks if the cache is full.
     * @return True if full, false otherwise.
     */
    bool IsFull() const;

    /**
     * @brief Gets the number of headers in the cache.
     * @return The number of headers.
     */
    size_t Size() const;

    /**
     * @brief Gets the maximum size of the cache.
     * @return The maximum size.
     */
    size_t MaxSize() const;

    /**
     * @brief Checks if the cache contains a header with the given hash.
     * @param hash The hash to check.
     * @return True if found, false otherwise.
     */
    bool Contains(const io::UInt256& hash) const;

    /**
     * @brief Removes a header from the cache by hash.
     * @param hash The hash of the header to remove.
     * @return True if removed, false if not found.
     */
    bool Remove(const io::UInt256& hash);

    /**
     * @brief Tries to remove the first header from the cache.
     * @return True if removed, false if cache is empty.
     */
    bool TryRemoveFirst();

    /**
     * @brief Clears all headers from the cache.
     */
    void Clear();

  private:
    mutable std::shared_mutex mutex_;
    std::deque<std::shared_ptr<Header>> headers_;
    std::unordered_map<io::UInt256, std::shared_ptr<Header>> hash_index_;
    std::unordered_map<uint32_t, std::shared_ptr<Header>> height_index_;
    size_t max_size_;
};

}  // namespace neo::ledger
