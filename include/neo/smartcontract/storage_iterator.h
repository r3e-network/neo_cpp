#pragma once

#include <neo/persistence/data_cache.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/byte_vector.h>
#include <memory>
#include <vector>
#include <utility>

namespace neo::smartcontract
{
    /**
     * @brief Iterator for storage items.
     * 
     * This class provides an iterator interface for storage items in the blockchain.
     * It is used by the System.Storage.Find system call to iterate over storage items
     * that match a given prefix.
     */
    class StorageIterator
    {
    public:
        /**
         * @brief Constructs a StorageIterator.
         * @param snapshot The data cache snapshot.
         * @param prefix The storage key prefix to match.
         */
        StorageIterator(std::shared_ptr<persistence::DataCache> snapshot, const persistence::StorageKey& prefix);

        /**
         * @brief Checks if there are more items in the iterator.
         * @return True if there are more items, false otherwise.
         */
        bool HasNext() const;

        /**
         * @brief Advances the iterator to the next item.
         * @return The key-value pair of the next item.
         * @throws std::runtime_error if there are no more items.
         */
        std::pair<io::ByteVector, io::ByteVector> Next();

        /**
         * @brief Gets the current key-value pair.
         * @return The current key-value pair.
         * @throws std::runtime_error if the iterator has not been advanced.
         */
        std::pair<io::ByteVector, io::ByteVector> GetCurrent() const;

    private:
        std::shared_ptr<persistence::DataCache> snapshot_;
        persistence::StorageKey prefix_;
        std::vector<std::pair<persistence::StorageKey, persistence::StorageItem>> entries_;
        size_t currentIndex_;
        std::pair<io::ByteVector, io::ByteVector> currentPair_;
    };
}
