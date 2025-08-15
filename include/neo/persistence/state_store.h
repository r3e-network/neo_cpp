#ifndef NEO_PERSISTENCE_STATE_STORE_H
#define NEO_PERSISTENCE_STATE_STORE_H

#include <neo/core/types.h>
#include <memory>
#include <vector>
#include <map>
#include <optional>

namespace neo {
namespace persistence {

    /**
     * @brief Key for storage operations
     */
    class StorageKey {
    public:
        StorageKey(uint32_t id, const std::vector<uint8_t>& key);
        
        uint32_t GetId() const { return id_; }
        const std::vector<uint8_t>& GetKey() const { return key_; }
        
        bool operator<(const StorageKey& other) const;
        bool operator==(const StorageKey& other) const;
        
        std::vector<uint8_t> Serialize() const;
        static StorageKey Deserialize(const std::vector<uint8_t>& data);
        
    private:
        uint32_t id_;
        std::vector<uint8_t> key_;
    };

    /**
     * @brief Value for storage operations
     */
    class StorageItem {
    public:
        StorageItem() = default;
        StorageItem(const std::vector<uint8_t>& value, bool is_constant = false);
        
        const std::vector<uint8_t>& GetValue() const { return value_; }
        bool IsConstant() const { return is_constant_; }
        
        void SetValue(const std::vector<uint8_t>& value) { value_ = value; }
        void SetConstant(bool constant) { is_constant_ = constant; }
        
        std::vector<uint8_t> Serialize() const;
        static StorageItem Deserialize(const std::vector<uint8_t>& data);
        
    private:
        std::vector<uint8_t> value_;
        bool is_constant_ = false;
    };

    /**
     * @brief Interface for state storage
     */
    class IStateStore {
    public:
        virtual ~IStateStore() = default;
        
        // Basic operations
        virtual void Put(const StorageKey& key, const StorageItem& value) = 0;
        virtual std::optional<StorageItem> Get(const StorageKey& key) const = 0;
        virtual void Delete(const StorageKey& key) = 0;
        virtual bool Contains(const StorageKey& key) const = 0;
        
        // Batch operations
        virtual void PutBatch(const std::map<StorageKey, StorageItem>& items) = 0;
        virtual void DeleteBatch(const std::vector<StorageKey>& keys) = 0;
        
        // Query operations
        virtual std::map<StorageKey, StorageItem> Find(const std::vector<uint8_t>& prefix) const = 0;
        virtual std::map<StorageKey, StorageItem> GetAll() const = 0;
        
        // Transaction support
        virtual void BeginTransaction() = 0;
        virtual void Commit() = 0;
        virtual void Rollback() = 0;
        
        // State management
        virtual void Clear() = 0;
        virtual size_t Size() const = 0;
    };

    /**
     * @brief In-memory implementation of state store
     */
    class MemoryStateStore : public IStateStore {
    public:
        MemoryStateStore() = default;
        
        void Put(const StorageKey& key, const StorageItem& value) override;
        std::optional<StorageItem> Get(const StorageKey& key) const override;
        void Delete(const StorageKey& key) override;
        bool Contains(const StorageKey& key) const override;
        
        void PutBatch(const std::map<StorageKey, StorageItem>& items) override;
        void DeleteBatch(const std::vector<StorageKey>& keys) override;
        
        std::map<StorageKey, StorageItem> Find(const std::vector<uint8_t>& prefix) const override;
        std::map<StorageKey, StorageItem> GetAll() const override;
        
        void BeginTransaction() override;
        void Commit() override;
        void Rollback() override;
        
        void Clear() override;
        size_t Size() const override;
        
    private:
        std::map<StorageKey, StorageItem> store_;
        std::map<StorageKey, StorageItem> transaction_store_;
        bool in_transaction_ = false;
    };

    /**
     * @brief State store factory
     */
    class StateStoreFactory {
    public:
        static std::unique_ptr<IStateStore> CreateMemoryStore();
        static std::unique_ptr<IStateStore> CreateLevelDBStore(const std::string& path);
        static std::unique_ptr<IStateStore> CreateRocksDBStore(const std::string& path);
    };

} // namespace persistence
} // namespace neo

#endif // NEO_PERSISTENCE_STATE_STORE_H