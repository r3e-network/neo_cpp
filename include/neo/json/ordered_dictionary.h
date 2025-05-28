#pragma once

#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace neo::json
{
    /**
     * @brief An ordered dictionary that maintains insertion order.
     * @tparam TKey The key type.
     * @tparam TValue The value type.
     */
    template<typename TKey, typename TValue>
    class OrderedDictionary
    {
    private:
        struct Item
        {
            TKey key;
            TValue value;
            
            Item(const TKey& k, const TValue& v) : key(k), value(v) {}
            Item(TKey&& k, TValue&& v) : key(std::move(k)), value(std::move(v)) {}
        };

        std::vector<Item> items_;
        std::unordered_map<TKey, size_t> key_to_index_;

    public:
        using iterator = typename std::vector<Item>::iterator;
        using const_iterator = typename std::vector<Item>::const_iterator;

        /**
         * @brief Default constructor.
         */
        OrderedDictionary() = default;

        /**
         * @brief Copy constructor.
         */
        OrderedDictionary(const OrderedDictionary& other) = default;

        /**
         * @brief Move constructor.
         */
        OrderedDictionary(OrderedDictionary&& other) noexcept = default;

        /**
         * @brief Copy assignment operator.
         */
        OrderedDictionary& operator=(const OrderedDictionary& other) = default;

        /**
         * @brief Move assignment operator.
         */
        OrderedDictionary& operator=(OrderedDictionary&& other) noexcept = default;

        /**
         * @brief Gets the number of elements.
         * @return The number of elements.
         */
        size_t size() const { return items_.size(); }

        /**
         * @brief Checks if the dictionary is empty.
         * @return True if empty, false otherwise.
         */
        bool empty() const { return items_.empty(); }

        /**
         * @brief Clears all elements.
         */
        void clear()
        {
            items_.clear();
            key_to_index_.clear();
        }

        /**
         * @brief Adds or updates an element.
         * @param key The key.
         * @param value The value.
         */
        void insert_or_assign(const TKey& key, const TValue& value)
        {
            auto it = key_to_index_.find(key);
            if (it != key_to_index_.end())
            {
                // Update existing
                items_[it->second].value = value;
            }
            else
            {
                // Add new
                size_t index = items_.size();
                items_.emplace_back(key, value);
                key_to_index_[key] = index;
            }
        }

        /**
         * @brief Adds or updates an element (move version).
         * @param key The key.
         * @param value The value.
         */
        void insert_or_assign(TKey&& key, TValue&& value)
        {
            auto it = key_to_index_.find(key);
            if (it != key_to_index_.end())
            {
                // Update existing
                items_[it->second].value = std::move(value);
            }
            else
            {
                // Add new
                size_t index = items_.size();
                key_to_index_[key] = index;
                items_.emplace_back(std::move(key), std::move(value));
            }
        }

        /**
         * @brief Checks if a key exists.
         * @param key The key to check.
         * @return True if the key exists, false otherwise.
         */
        bool contains(const TKey& key) const
        {
            return key_to_index_.find(key) != key_to_index_.end();
        }

        /**
         * @brief Gets a value by key.
         * @param key The key.
         * @return Reference to the value.
         * @throws std::out_of_range if key not found.
         */
        TValue& at(const TKey& key)
        {
            auto it = key_to_index_.find(key);
            if (it == key_to_index_.end())
                throw std::out_of_range("Key not found");
            return items_[it->second].value;
        }

        /**
         * @brief Gets a value by key (const version).
         * @param key The key.
         * @return Const reference to the value.
         * @throws std::out_of_range if key not found.
         */
        const TValue& at(const TKey& key) const
        {
            auto it = key_to_index_.find(key);
            if (it == key_to_index_.end())
                throw std::out_of_range("Key not found");
            return items_[it->second].value;
        }

        /**
         * @brief Gets a value by index.
         * @param index The index.
         * @return Reference to the value.
         * @throws std::out_of_range if index is invalid.
         */
        TValue& at(size_t index)
        {
            if (index >= items_.size())
                throw std::out_of_range("Index out of range");
            return items_[index].value;
        }

        /**
         * @brief Gets a value by index (const version).
         * @param index The index.
         * @return Const reference to the value.
         * @throws std::out_of_range if index is invalid.
         */
        const TValue& at(size_t index) const
        {
            if (index >= items_.size())
                throw std::out_of_range("Index out of range");
            return items_[index].value;
        }

        /**
         * @brief Operator[] for key access.
         * @param key The key.
         * @return Reference to the value.
         */
        TValue& operator[](const TKey& key)
        {
            auto it = key_to_index_.find(key);
            if (it != key_to_index_.end())
            {
                return items_[it->second].value;
            }
            else
            {
                // Insert default value
                size_t index = items_.size();
                items_.emplace_back(key, TValue{});
                key_to_index_[key] = index;
                return items_[index].value;
            }
        }

        /**
         * @brief Operator[] for index access.
         * @param index The index.
         * @return Reference to the value.
         */
        TValue& operator[](size_t index)
        {
            return items_[index].value;
        }

        /**
         * @brief Operator[] for index access (const version).
         * @param index The index.
         * @return Const reference to the value.
         */
        const TValue& operator[](size_t index) const
        {
            return items_[index].value;
        }

        /**
         * @brief Removes an element by key.
         * @param key The key to remove.
         * @return True if removed, false if not found.
         */
        bool erase(const TKey& key)
        {
            auto it = key_to_index_.find(key);
            if (it == key_to_index_.end())
                return false;

            size_t index = it->second;
            
            // Remove from items
            items_.erase(items_.begin() + index);
            
            // Remove from key map
            key_to_index_.erase(it);
            
            // Update indices in key map
            for (auto& pair : key_to_index_)
            {
                if (pair.second > index)
                    pair.second--;
            }
            
            return true;
        }

        /**
         * @brief Gets an iterator to the beginning.
         * @return Iterator to the beginning.
         */
        iterator begin() { return items_.begin(); }

        /**
         * @brief Gets an iterator to the end.
         * @return Iterator to the end.
         */
        iterator end() { return items_.end(); }

        /**
         * @brief Gets a const iterator to the beginning.
         * @return Const iterator to the beginning.
         */
        const_iterator begin() const { return items_.begin(); }

        /**
         * @brief Gets a const iterator to the end.
         * @return Const iterator to the end.
         */
        const_iterator end() const { return items_.end(); }

        /**
         * @brief Gets a const iterator to the beginning.
         * @return Const iterator to the beginning.
         */
        const_iterator cbegin() const { return items_.cbegin(); }

        /**
         * @brief Gets a const iterator to the end.
         * @return Const iterator to the end.
         */
        const_iterator cend() const { return items_.cend(); }

        /**
         * @brief Gets the key at the specified index.
         * @param index The index.
         * @return The key at the index.
         */
        const TKey& key_at(size_t index) const
        {
            if (index >= items_.size())
                throw std::out_of_range("Index out of range");
            return items_[index].key;
        }

        /**
         * @brief Gets the value at the specified index.
         * @param index The index.
         * @return The value at the index.
         */
        const TValue& value_at(size_t index) const
        {
            if (index >= items_.size())
                throw std::out_of_range("Index out of range");
            return items_[index].value;
        }
    };
}
