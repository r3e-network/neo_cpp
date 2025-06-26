#pragma once

#include <neo/persistence/store_view.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>

namespace neo::persistence
{
    /**
     * @brief In-memory implementation of store view for testing and temporary storage
     */
    class MemoryStoreView : public StoreView
    {
    private:
        std::unordered_map<StorageKey, StorageItem> storage_;
        
    public:
        /**
         * @brief Default constructor
         */
        MemoryStoreView() = default;
        
        /**
         * @brief Virtual destructor
         */
        virtual ~MemoryStoreView() = default;
        
        // StoreView interface implementation
        std::optional<StorageItem> TryGet(const StorageKey& key) const override;
        std::shared_ptr<StorageItem> TryGet(const StorageKey& key) override;
        std::shared_ptr<StorageItem> GetAndChange(const StorageKey& key, std::function<std::shared_ptr<StorageItem>()> factory = nullptr) override;
        void Add(const StorageKey& key, const StorageItem& item) override;
        void Delete(const StorageKey& key) override;
        std::vector<std::pair<StorageKey, StorageItem>> Find(const StorageKey* prefix = nullptr) const override;
        std::unique_ptr<StorageIterator> Seek(const StorageKey& prefix) const override;
        void Commit() override;
        std::shared_ptr<StoreView> CreateSnapshot() override;
        
        /**
         * @brief Clear all items
         */
        void Clear();
        
        /**
         * @brief Get number of items
         * @return Number of items in storage
         */
        size_t Size() const { return storage_.size(); }
        
        /**
         * @brief Check if storage is empty
         * @return True if empty
         */
        bool IsEmpty() const { return storage_.empty(); }
    };
}