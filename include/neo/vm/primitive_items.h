/**
 * @file primitive_items.h
 * @brief Primitive Items
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/stack_item.h>

#include <neo/io/byte_span.h>

namespace neo::vm
{
/**
 * @brief Represents a boolean stack item.
 */
class BooleanItem : public StackItem
{
   public:
    /**
     * @brief Constructs a BooleanItem.
     * @param value The value.
     */
    explicit BooleanItem(bool value);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const { return std::const_pointer_cast<StackItem>(shared_from_this()); }

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
     * @brief Gets the byte array value of the stack item.
     * @return The byte array value of the stack item.
     */
    io::ByteVector GetByteArray() const override;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

   private:
    bool value_;
};

/**
 * @brief Represents an integer stack item.
 */
class IntegerItem : public StackItem
{
   public:
    /**
     * @brief Constructs an IntegerItem.
     * @param value The value.
     */
    explicit IntegerItem(int64_t value);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const { return std::const_pointer_cast<StackItem>(shared_from_this()); }

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
     * @brief Gets the byte array value of the stack item.
     * @return The byte array value of the stack item.
     */
    io::ByteVector GetByteArray() const override;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

   private:
    int64_t value_;
};

/**
 * @brief Represents a byte string stack item.
 */
class ByteStringItem : public StackItem
{
   public:
    /**
     * @brief Constructs a ByteStringItem.
     * @param value The value.
     */
    explicit ByteStringItem(const io::ByteVector& value);

    /**
     * @brief Constructs a ByteStringItem.
     * @param value The value.
     */
    explicit ByteStringItem(const io::ByteSpan& value);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const { return std::const_pointer_cast<StackItem>(shared_from_this()); }

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
     * @brief Gets the byte array value of the stack item.
     * @return The byte array value of the stack item.
     */
    io::ByteVector GetByteArray() const override;

    /**
     * @brief Gets a read-only view over the underlying bytes.
     * @return A ByteSpan referencing the stored bytes.
     */
    io::ByteSpan GetByteSpan() const;

    /**
     * @brief Gets the string value of the stack item.
     * @return The string value of the stack item.
     */
    std::string GetString() const override;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

   private:
    io::ByteVector value_;
};

/**
 * @brief Represents a buffer stack item.
 */
class BufferItem : public StackItem
{
   public:
    /**
     * @brief Constructs a BufferItem.
     * @param value The value.
     */
    explicit BufferItem(const io::ByteVector& value);

    /**
     * @brief Constructs a BufferItem.
     * @param value The value.
     */
    explicit BufferItem(const io::ByteSpan& value);

    /**
     * @brief Conversion operator to std::shared_ptr<StackItem>.
     * @return A shared pointer to the base StackItem.
     */
    operator std::shared_ptr<StackItem>() const { return std::const_pointer_cast<StackItem>(shared_from_this()); }

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
     * @brief Gets the byte array value of the stack item.
     * @return The byte array value of the stack item.
     */
    io::ByteVector GetByteArray() const override;

    /**
     * @brief Returns a mutable span over the underlying buffer.
     * @return Reference to the mutable byte vector.
     */
    io::ByteVector& GetSpan();

    /**
     * @brief Returns a read-only view over the underlying buffer.
     * @return Const reference to the byte vector.
     */
    const io::ByteVector& GetSpan() const;

    /**
     * @brief Gets the string value of the stack item.
     * @return The string value of the stack item.
     */
    std::string GetString() const override;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool Equals(const StackItem& other) const override;

   private:
    io::ByteVector value_;
};
}  // namespace neo::vm
