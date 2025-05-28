#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <optional>
#include <vector>
#include <utility>
#include <memory>
#include <string>

namespace neo::persistence
{
    /**
     * @brief Enum for seek direction.
     */
    enum class SeekDirection
    {
        Forward,
        Backward
    };

    /**
     * @brief Interface for read-only operations on a key-value store.
     * @tparam TKey The type of the keys.
     * @tparam TValue The type of the values.
     */
    template <typename TKey, typename TValue>
    class IReadOnlyStore
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IReadOnlyStore() = default;

        /**
         * @brief Tries to get a value from the store.
         * @param key The key to look up.
         * @return The value if found, std::nullopt otherwise.
         */
        virtual std::optional<TValue> TryGet(const TKey& key) const = 0;

        /**
         * @brief Checks if the store contains a key.
         * @param key The key to check.
         * @return True if the key exists, false otherwise.
         */
        virtual bool Contains(const TKey& key) const = 0;

        /**
         * @brief Finds all key-value pairs with keys that start with the specified prefix.
         * @param prefix The prefix to search for. If nullptr, all key-value pairs are returned.
         * @param direction The direction to seek.
         * @return The key-value pairs found.
         */
        virtual std::vector<std::pair<TKey, TValue>> Find(const TKey* prefix = nullptr, SeekDirection direction = SeekDirection::Forward) const = 0;
    };

    /**
     * @brief Interface for write operations on a key-value store.
     * @tparam TKey The type of the keys.
     * @tparam TValue The type of the values.
     */
    template <typename TKey, typename TValue>
    class IWriteStore
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IWriteStore() = default;

        /**
         * @brief Puts a value in the store.
         * @param key The key.
         * @param value The value.
         */
        virtual void Put(const TKey& key, const TValue& value) = 0;

        /**
         * @brief Deletes a value from the store.
         * @param key The key.
         */
        virtual void Delete(const TKey& key) = 0;

        /**
         * @brief Puts a value in the store and syncs to disk.
         * @param key The key.
         * @param value The value.
         */
        virtual void PutSync(const TKey& key, const TValue& value) { Put(key, value); }
    };

    /**
     * @brief Interface for a key-value store.
     */
    class IStore : public IReadOnlyStore<io::ByteVector, io::ByteVector>, public IWriteStore<io::ByteVector, io::ByteVector>
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IStore() = default;
    };

    /**
     * @brief Interface for a snapshot of a key-value store.
     */
    class IStoreSnapshot : public IStore
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IStoreSnapshot() = default;

        /**
         * @brief Commits the changes to the store.
         */
        virtual void Commit() = 0;

        /**
         * @brief Gets the underlying store.
         * @return The underlying store.
         */
        virtual IStore& GetStore() = 0;
    };

    /**
     * @brief Interface for a store provider.
     */
    class IStoreProvider
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IStoreProvider() = default;

        /**
         * @brief Gets the name of the store provider.
         * @return The name of the store provider.
         */
        virtual std::string GetName() const = 0;

        /**
         * @brief Gets a store.
         * @param path The path to the store.
         * @return The store.
         */
        virtual std::unique_ptr<IStore> GetStore(const std::string& path) = 0;
    };
}
