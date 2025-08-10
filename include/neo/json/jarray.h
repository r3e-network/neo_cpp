#pragma once

#include <neo/json/jtoken.h>

#include <vector>

namespace neo::json
{
/**
 * @brief Represents a JSON array.
 */
class JArray : public JToken
{
   public:
    using Items = std::vector<std::shared_ptr<JToken>>;

    /**
     * @brief Default constructor.
     */
    JArray();

    /**
     * @brief Constructor with initial items.
     * @param items The initial items.
     */
    explicit JArray(const Items& items);

    /**
     * @brief Constructor with initializer list.
     * @param items The initial items.
     */
    JArray(std::initializer_list<std::shared_ptr<JToken>> items);

    /**
     * @brief Gets the type of this token.
     * @return JTokenType::Array.
     */
    JTokenType GetType() const override;

    /**
     * @brief Gets or sets the child token at the specified index.
     * @param index The zero-based index of the child token to get or set.
     * @return The child token at the specified index.
     * @throws std::out_of_range if index is invalid.
     */
    std::shared_ptr<JToken> operator[](int index) const override;

    /**
     * @brief Converts this token to a string representation.
     * @return The JSON string representation.
     */
    std::string ToString() const override;

    /**
     * @brief Clones this token.
     * @return A clone of this token.
     */
    std::shared_ptr<JToken> Clone() const override;

    /**
     * @brief Checks if this token equals another token.
     * @param other The other token.
     * @return True if equal, false otherwise.
     */
    bool Equals(const JToken& other) const override;

    /**
     * @brief Adds an item to the array.
     * @param item The item to add.
     */
    void Add(std::shared_ptr<JToken> item);

    /**
     * @brief Removes an item at the specified index.
     * @param index The index of the item to remove.
     * @throws std::out_of_range if index is invalid.
     */
    void RemoveAt(int index);

    /**
     * @brief Clears all items from the array.
     */
    void Clear();

    /**
     * @brief Gets the number of items in the array.
     * @return The number of items.
     */
    size_t Count() const;

    /**
     * @brief Checks if the array is empty.
     * @return True if empty, false otherwise.
     */
    bool IsEmpty() const;

    /**
     * @brief Gets the items.
     * @return The items.
     */
    const Items& GetItems() const;

    /**
     * @brief Gets the items (mutable).
     * @return The items.
     */
    Items& GetItems();

    /**
     * @brief Gets an iterator to the beginning.
     * @return Iterator to the beginning.
     */
    Items::iterator begin();

    /**
     * @brief Gets an iterator to the end.
     * @return Iterator to the end.
     */
    Items::iterator end();

    /**
     * @brief Gets a const iterator to the beginning.
     * @return Const iterator to the beginning.
     */
    Items::const_iterator begin() const;

    /**
     * @brief Gets a const iterator to the end.
     * @return Const iterator to the end.
     */
    Items::const_iterator end() const;

    /**
     * @brief Gets a const iterator to the beginning.
     * @return Const iterator to the beginning.
     */
    Items::const_iterator cbegin() const;

    /**
     * @brief Gets a const iterator to the end.
     * @return Const iterator to the end.
     */
    Items::const_iterator cend() const;

   protected:
    /**
     * @brief Writes this token to JSON output.
     * @param output The output string.
     * @param indented Whether to use indented formatting.
     * @param indent_level The indentation level.
     */
    void WriteJson(std::string& output, bool indented = false, int indent_level = 0) const override;

   private:
    Items items_;
};
}  // namespace neo::json
