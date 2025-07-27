#pragma once

#include <neo/vm/stack_item.h>
#include <optional>

namespace neo::vm
{
/**
 * @brief Represents an array stack item.
 */
class ArrayItem : public StackItem
{
  public:
    /**
     * @brief Constructs an ArrayItem.
     * @param value The value.
     * @param refCounter The reference counter.
     */
    explicit ArrayItem(const std::vector<std::shared_ptr<StackItem>>& value, ReferenceCounter* refCounter = nullptr);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const
    {
        return std::const_pointer_cast<StackItem>(shared_from_this());
    }

    /**
     * @brief Destructor.
     */
    ~ArrayItem() override;

    /**
     * @brief Initialize reference counting after construction.
     * This must be called after the shared_ptr is fully constructed.
     */
    void InitializeReferences();

    /**
     * @brief Gets the type of the stack item.
     * @return The type of the stack item.
     */
    StackItemType GetType() const override;

    /**
     * @brief Gets the boolean value of the stack item.
     * @return The boolean value of the stack item.
     */
    bool GetBoolean() const override;

    /**
     * @brief Gets the integer value of the stack item.
     * @return The integer value of the stack item.
     */
    int64_t GetInteger() const override;

    /**
     * @brief Gets the array value of the stack item.
     * @return The array value of the stack item.
     */
    std::vector<std::shared_ptr<StackItem>> GetArray() const override;

    /**
     * @brief Gets the struct value of the stack item.
     * @return The struct value of the stack item.
     */
    std::vector<std::shared_ptr<StackItem>> GetStruct() const override;

    /**
     * @brief Adds an item to the array.
     * @param item The item to add.
     */
    void Add(std::shared_ptr<StackItem> item) override;

    /**
     * @brief Gets an item from the array.
     * @param index The index.
     * @return The item.
     */
    std::shared_ptr<StackItem> Get(size_t index) const;

    /**
     * @brief Sets an item in the array.
     * @param index The index.
     * @param item The item.
     */
    void Set(size_t index, std::shared_ptr<StackItem> item);

    /**
     * @brief Removes an item from the array.
     * @param index The index.
     */
    void Remove(size_t index);

    /**
     * @brief Gets the size of the array.
     * @return The size of the array.
     */
    size_t Size() const override;

    /**
     * @brief Clears the array.
     */
    void Clear();

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

    /**
     * @brief Creates a deep copy of the stack item.
     * @param refCounter The reference counter.
     * @param asImmutable Whether to create an immutable copy.
     * @return The deep copy.
     */
    std::shared_ptr<StackItem> DeepCopy(ReferenceCounter* refCounter = nullptr,
                                        bool asImmutable = false) const override;

  protected:
    std::vector<std::shared_ptr<StackItem>> value_;
    ReferenceCounter* refCounter_ = nullptr;
};

/**
 * @brief Represents a struct stack item.
 */
class StructItem : public ArrayItem
{
  public:
    /**
     * @brief Constructs a StructItem.
     * @param value The value.
     * @param refCounter The reference counter.
     */
    explicit StructItem(const std::vector<std::shared_ptr<StackItem>>& value, ReferenceCounter* refCounter = nullptr);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const
    {
        return std::const_pointer_cast<StackItem>(shared_from_this());
    }

    /**
     * @brief Conversion operator to std::shared_ptr<ArrayItem>.
     * @return A shared pointer to the base ArrayItem.
     */
    operator std::shared_ptr<ArrayItem>() const
    {
        return std::dynamic_pointer_cast<ArrayItem>(std::const_pointer_cast<StackItem>(shared_from_this()));
    }

    /**
     * @brief Gets the type of the stack item.
     * @return The type of the stack item.
     */
    StackItemType GetType() const override;

    /**
     * @brief Gets the boolean value of the stack item.
     * @return The boolean value of the stack item.
     */
    bool GetBoolean() const override;

    /**
     * @brief Gets the count of items in the struct.
     * @return The count of items.
     */
    size_t Count() const
    {
        return Size();
    }

    /**
     * @brief Clones the struct.
     * @return The cloned struct.
     */
    std::shared_ptr<StructItem> Clone() const;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

    /**
     * @brief Creates a deep copy of the stack item.
     * @param refCounter The reference counter.
     * @param asImmutable Whether to create an immutable copy.
     * @return The deep copy.
     */
    std::shared_ptr<StackItem> DeepCopy(ReferenceCounter* refCounter = nullptr,
                                        bool asImmutable = false) const override;
};

/**
 * @brief Custom comparator for StackItem shared_ptr in maps.
 * Compares pointer addresses for ordering. All actual key comparisons
 * are done manually using StackItem::Equals() method.
 */
struct StackItemPtrComparator
{
    bool operator()(const std::shared_ptr<StackItem>& lhs, const std::shared_ptr<StackItem>& rhs) const
    {
        return lhs.get() < rhs.get();
    }
};

/**
 * @brief Represents a map stack item.
 */
class MapItem : public StackItem
{
  public:
    /**
     * @brief Constructs a MapItem.
     * @param value The value.
     * @param refCounter The reference counter.
     */
    explicit MapItem(
        const std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>, StackItemPtrComparator>& value = {},
        ReferenceCounter* refCounter = nullptr);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const
    {
        return std::const_pointer_cast<StackItem>(shared_from_this());
    }

    /**
     * @brief Destructor.
     */
    ~MapItem() override;

    /**
     * @brief Gets the type of the stack item.
     * @return The type of the stack item.
     */
    StackItemType GetType() const override;

    /**
     * @brief Gets the boolean value of the stack item.
     * @return The boolean value of the stack item.
     */
    bool GetBoolean() const override;

    /**
     * @brief Gets the map value of the stack item.
     * @return The map value of the stack item.
     */
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> GetMap() const override;

    /**
     * @brief Gets the size of the map.
     * @return The size of the map.
     */
    size_t GetSize() const;

    /**
     * @brief Gets an item from the map.
     * @param key The key.
     * @return The item.
     */
    std::optional<std::shared_ptr<StackItem>> Get(const std::shared_ptr<StackItem>& key) const;

    /**
     * @brief Sets an item in the map.
     * @param key The key.
     * @param value The value.
     */
    void Set(const std::shared_ptr<StackItem>& key, const std::shared_ptr<StackItem>& value);

    /**
     * @brief Removes an item from the map.
     * @param key The key.
     */
    void Remove(const std::shared_ptr<StackItem>& key);

    /**
     * @brief Gets the size of the map.
     * @return The size of the map.
     */
    size_t Size() const override;

    /**
     * @brief Clears the map.
     */
    void Clear();

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

    /**
     * @brief Creates a deep copy of the stack item.
     * @param refCounter The reference counter.
     * @param asImmutable Whether to create an immutable copy.
     * @return The deep copy.
     */
    std::shared_ptr<StackItem> DeepCopy(ReferenceCounter* refCounter = nullptr,
                                        bool asImmutable = false) const override;

  private:
    std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>, StackItemPtrComparator> value_;
    ReferenceCounter* refCounter_ = nullptr;
};

// Type alias for compatibility with C# naming
using Struct = StructItem;
}  // namespace neo::vm
