#pragma once

#include <neo/io/caching/lru_cache.h>
#include <neo/smartcontract/contract_state.h>
#include <neo/io/uint160.h>
#include <functional>

namespace neo::io::caching
{
    /**
     * @brief A cache for ContractState objects.
     */
    class ContractCache
    {
    public:
        /**
         * @brief Constructs a ContractCache with the specified capacity.
         * @param capacity The maximum number of items the cache can hold.
         */
        explicit ContractCache(size_t capacity)
            : cache_(capacity)
        {
        }

        /**
         * @brief Adds a contract to the cache.
         * @param contract The contract to add.
         */
        void Add(std::shared_ptr<smartcontract::ContractState> contract)
        {
            if (!contract)
                return;
            
            // Add to cache
            cache_.Add(contract->GetScriptHash(), contract);
        }

        /**
         * @brief Gets a contract by script hash.
         * @param scriptHash The script hash of the contract.
         * @return The contract if found, std::nullopt otherwise.
         */
        std::optional<std::shared_ptr<smartcontract::ContractState>> Get(const UInt160& scriptHash)
        {
            return cache_.Get(scriptHash);
        }

        /**
         * @brief Tries to get a contract by script hash.
         * @param scriptHash The script hash of the contract.
         * @param contract The contract if found.
         * @return True if the contract was found, false otherwise.
         */
        bool TryGet(const UInt160& scriptHash, std::shared_ptr<smartcontract::ContractState>& contract)
        {
            return cache_.TryGet(scriptHash, contract);
        }

        /**
         * @brief Removes a contract by script hash.
         * @param scriptHash The script hash of the contract.
         * @return True if the contract was removed, false otherwise.
         */
        bool Remove(const UInt160& scriptHash)
        {
            return cache_.Remove(scriptHash);
        }

        /**
         * @brief Clears the cache.
         */
        void Clear()
        {
            cache_.Clear();
        }

        /**
         * @brief Gets the number of items in the cache.
         * @return The number of items in the cache.
         */
        size_t Size() const
        {
            return cache_.Size();
        }

        /**
         * @brief Gets the capacity of the cache.
         * @return The capacity of the cache.
         */
        size_t Capacity() const
        {
            return cache_.Capacity();
        }

    private:
        LRUCache<UInt160, std::shared_ptr<smartcontract::ContractState>> cache_;
    };
}
