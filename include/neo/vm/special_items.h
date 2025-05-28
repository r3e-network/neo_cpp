#pragma once

#include <neo/vm/stack_item.h>

namespace neo::vm
{
    /**
     * @brief Represents an interop interface stack item.
     */
    class InteropInterfaceItem : public StackItem
    {
    public:
        /**
         * @brief Constructs an InteropInterfaceItem.
         * @param value The value.
         */
        explicit InteropInterfaceItem(std::shared_ptr<void> value);

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
         * @brief Gets the interop interface value of the stack item.
         * @return The interop interface value of the stack item.
         */
        std::shared_ptr<void> GetInterface() const override;

        /**
         * @brief Gets the size of the stack item.
         * @return The size of the stack item.
         */
        size_t Size() const override;

        /**
         * @brief Gets the hash code of the stack item.
         * @return The hash code of the stack item.
         */
        size_t GetHashCode() const override;

        /**
         * @brief Checks if this stack item is equal to another stack item.
         * @param other The other stack item.
         * @return True if the stack items are equal, false otherwise.
         */
        bool Equals(const StackItem& other) const override;

    private:
        std::shared_ptr<void> value_;
    };

    /**
     * @brief Represents a pointer stack item.
     */
    class PointerItem : public StackItem
    {
    public:
        /**
         * @brief Constructs a PointerItem.
         * @param position The position.
         * @param value The value (optional).
         */
        explicit PointerItem(int32_t position, std::shared_ptr<StackItem> value = nullptr);

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
         * @brief Gets the position.
         * @return The position.
         */
        int32_t GetPosition() const;

        /**
         * @brief Gets the value.
         * @return The value.
         */
        std::shared_ptr<StackItem> GetValue() const;

        /**
         * @brief Checks if this stack item is equal to another stack item.
         * @param other The other stack item.
         * @return True if the stack items are equal, false otherwise.
         */
        bool Equals(const StackItem& other) const override;

    private:
        int32_t position_;
        std::shared_ptr<StackItem> value_;
    };

    /**
     * @brief Represents a null stack item.
     */
    class NullItem : public StackItem
    {
    public:
        /**
         * @brief Constructs a NullItem.
         */
        NullItem() = default;

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
         * @brief Converts the stack item to the specified type.
         * @param type The type to convert to.
         * @return The converted stack item.
         */
        std::shared_ptr<StackItem> ConvertTo(StackItemType type) const override;

        /**
         * @brief Checks if this stack item is equal to another stack item.
         * @param other The other stack item.
         * @return True if the stack items are equal, false otherwise.
         */
        bool Equals(const StackItem& other) const override;
    };
}
