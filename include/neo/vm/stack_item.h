#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/stack_item_types.h>
#include <string>
#include <vector>

// Forward declarations
namespace neo::vm
{
class ReferenceCounter;
class IReferenceCounter;
}  // namespace neo::vm

namespace neo::io
{
class BinaryReader;
class BinaryWriter;
}  // namespace neo::io

namespace neo::core
{
class BigDecimal;
}

namespace neo::vm
{
// Forward declarations
namespace types
{
class Array;
class Boolean;
class Buffer;
class ByteString;
class CompoundType;
class Integer;
class InteropInterface;
class Map;
class Null;
class Pointer;
class PrimitiveType;
class Struct;
}  // namespace types

class StackItem;
class BooleanItem;
class IntegerItem;
class ByteStringItem;
class BufferItem;
class ArrayItem;
class StructItem;
class MapItem;
class InteropInterfaceItem;
class PointerItem;
class NullItem;

/**
 * @brief Represents a stack item in the VM.
 */
class StackItem : public std::enable_shared_from_this<StackItem>
{
  private:
    // Tarjan algorithm fields
    int dfn_ = -1;
    int low_link_ = -1;
    bool on_stack_ = false;

  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~StackItem() = default;

    /**
     * @brief Gets the type of the stack item.
     * @return The type of the stack item.
     */
    virtual StackItemType GetType() const = 0;

    /**
     * @brief Gets the boolean value of the stack item.
     * @return The boolean value of the stack item.
     */
    virtual bool GetBoolean() const = 0;

    /**
     * @brief Checks if this stack item is a boolean.
     * @return True if this is a boolean stack item.
     */
    virtual bool IsBoolean() const
    {
        return GetType() == StackItemType::Boolean;
    }

    /**
     * @brief Checks if this stack item is an integer.
     * @return True if this is an integer stack item.
     */
    virtual bool IsInteger() const
    {
        return GetType() == StackItemType::Integer;
    }

    /**
     * @brief Checks if this stack item is a byte string.
     * @return True if this is a byte string stack item.
     */
    virtual bool IsByteString() const
    {
        return GetType() == StackItemType::ByteString;
    }

    /**
     * @brief Checks if this stack item is a map.
     * @return True if this is a map stack item.
     */
    virtual bool IsMap() const
    {
        return GetType() == StackItemType::Map;
    }

    /**
     * @brief Checks if this stack item is an interop interface.
     * @return True if this is an interop interface stack item.
     */
    virtual bool IsInteropInterface() const
    {
        return GetType() == StackItemType::InteropInterface;
    }

    /**
     * @brief Gets the integer value of the stack item.
     * @return The integer value of the stack item.
     */
    virtual int64_t GetInteger() const;

    /**
     * @brief Gets the byte array value of the stack item.
     * @return The byte array value of the stack item.
     */
    virtual io::ByteVector GetByteArray() const;

    /**
     * @brief Gets the string value of the stack item.
     * @return The string value of the stack item.
     */
    virtual std::string GetString() const;

    /**
     * @brief Gets the array value of the stack item.
     * @return The array value of the stack item.
     */
    virtual std::vector<std::shared_ptr<StackItem>> GetArray() const;

    /**
     * @brief Gets the map value of the stack item.
     * @return The map value of the stack item.
     */
    virtual std::map<std::shared_ptr<StackItem>, std::shared_ptr<StackItem>> GetMap() const;

    /**
     * @brief Gets the interop interface value of the stack item.
     * @return The interop interface value of the stack item.
     */
    virtual std::shared_ptr<void> GetInterface() const;

    /**
     * @brief Gets the size of the stack item.
     * @return The size of the stack item.
     */
    virtual size_t Size() const;

    /**
     * @brief Converts the stack item to the specified type.
     * @param type The type to convert to.
     * @return The converted stack item.
     */
    virtual std::shared_ptr<StackItem> ConvertTo(StackItemType type) const;

    /**
     * @brief Gets the hash code of the stack item.
     * @return The hash code of the stack item.
     */
    virtual size_t GetHashCode() const;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    virtual bool Equals(const StackItem& other) const = 0;

    /**
     * @brief Checks if this stack item is equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are equal, false otherwise.
     */
    bool operator==(const StackItem& other) const;

    /**
     * @brief Checks if this stack item is not equal to another stack item.
     * @param other The other stack item.
     * @return True if the stack items are not equal, false otherwise.
     */
    bool operator!=(const StackItem& other) const;

    /**
     * @brief Compares this stack item to another stack item.
     * @param other The other stack item.
     * @return A negative value if this item is less than the other, 0 if they are equal, and a positive value if this
     * item is greater than the other.
     */
    virtual int CompareTo(const std::shared_ptr<StackItem>& other) const;

    /**
     * @brief Creates a deep copy of the stack item.
     * @param refCounter The reference counter.
     * @param asImmutable Whether to create an immutable copy.
     * @return The deep copy.
     */
    virtual std::shared_ptr<StackItem> DeepCopy(ReferenceCounter* refCounter = nullptr, bool asImmutable = false) const;

    /**
     * @brief Checks if this stack item is null.
     * @return True if the stack item is null, false otherwise.
     */
    bool IsNull() const;

    /**
     * @brief Checks if this stack item is an interop interface.
     * @return True if the stack item is an interop interface, false otherwise.
     */
    bool IsInterop() const;

    /**
     * @brief Checks if this stack item is an array.
     * @return True if the stack item is an array, false otherwise.
     */
    bool IsArray() const;

    /**
     * @brief Checks if this stack item is a struct.
     * @return True if the stack item is a struct, false otherwise.
     */
    bool IsStruct() const;

    /**
     * @brief Gets the null stack item.
     * @return The null stack item.
     */
    static std::shared_ptr<StackItem> Null();

    /**
     * @brief Gets the true stack item.
     * @return The true stack item.
     */
    static std::shared_ptr<StackItem> True();

    /**
     * @brief Gets the false stack item.
     * @return The false stack item.
     */
    static std::shared_ptr<StackItem> False();

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(bool value);

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(int64_t value);

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(const io::ByteVector& value);

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(const io::ByteSpan& value);

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(const std::string& value);

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(const io::UInt160& value);

    /**
     * @brief Creates a primitive stack item.
     * @param value The value.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(const io::UInt256& value);

    /**
     * @brief Creates a stack item from an array of stack items.
     * @param value The array of stack items.
     * @return The stack item.
     */
    static std::shared_ptr<StackItem> Create(const std::vector<std::shared_ptr<StackItem>>& value);

    /**
     * @brief Creates an empty array stack item.
     * @return The array stack item.
     */
    static std::shared_ptr<StackItem> CreateArray();

    /**
     * @brief Creates an array stack item from a vector of stack items.
     * @param items The items to include in the array.
     * @return The array stack item.
     */
    static std::shared_ptr<StackItem> CreateArray(const std::vector<std::shared_ptr<StackItem>>& items);

    /**
     * @brief Creates an empty struct stack item.
     * @param refCounter The reference counter.
     * @return The struct stack item.
     */
    static std::shared_ptr<StackItem> CreateStruct(ReferenceCounter& refCounter);

    /**
     * @brief Creates a struct stack item from a vector of stack items.
     * @param items The items to include in the struct.
     * @param refCounter The reference counter.
     * @return The struct stack item.
     */
    static std::shared_ptr<StackItem> CreateStruct(const std::vector<std::shared_ptr<StackItem>>& items,
                                                   ReferenceCounter& refCounter);

    /**
     * @brief Gets the struct value of the stack item.
     * @return The struct value of the stack item.
     */
    virtual std::vector<std::shared_ptr<StackItem>> GetStruct() const;

    /**
     * @brief Adds an item to this stack item (if it's an array or struct).
     * @param item The item to add.
     */
    virtual void Add(std::shared_ptr<StackItem> item);

    /**
     * @brief Deserializes a StackItem from a binary reader.
     * @param reader The binary reader.
     * @return The deserialized StackItem.
     */
    static std::shared_ptr<StackItem> Deserialize(io::BinaryReader& reader);

    /**
     * @brief Serializes a StackItem to a binary writer.
     * @param item The StackItem to serialize.
     * @param writer The binary writer.
     */
    static void Serialize(std::shared_ptr<StackItem> item, io::BinaryWriter& writer);

    /**
     * @brief Resets the Tarjan algorithm fields.
     */
    void Reset();

    /**
     * @brief Gets the DFN (Depth-First Number) of the stack item.
     * @return The DFN.
     */
    int GetDFN() const;

    /**
     * @brief Sets the DFN (Depth-First Number) of the stack item.
     * @param dfn The DFN.
     */
    void SetDFN(int dfn);

    /**
     * @brief Gets the low link value of the stack item.
     * @return The low link value.
     */
    int GetLowLink() const;

    /**
     * @brief Sets the low link value of the stack item.
     * @param low_link The low link value.
     */
    void SetLowLink(int low_link);

    /**
     * @brief Checks if the stack item is on the stack.
     * @return True if the stack item is on the stack, false otherwise.
     */
    bool IsOnStack() const;

    /**
     * @brief Sets whether the stack item is on the stack.
     * @param on_stack Whether the stack item is on the stack.
     */
    void SetOnStack(bool on_stack);

    /**
     * @brief Creates a map stack item.
     * @return The created map.
     */
    static std::shared_ptr<StackItem> CreateMap();

    /**
     * @brief Creates a byte string stack item.
     * @param data The byte data.
     * @return The created byte string.
     */
    static std::shared_ptr<StackItem> CreateByteString(const std::vector<uint8_t>& data);

    /**
     * @brief Creates a boolean stack item.
     * @param value The boolean value.
     * @return The created boolean.
     */
    static std::shared_ptr<StackItem> CreateBoolean(bool value);

    /**
     * @brief Sets the value of the stack item.
     * @param value The new value to set.
     */
    virtual void SetValue(const std::vector<uint8_t>& /* value */) {}

    /**
     * @brief Sets the value of the stack item from another stack item.
     * @param other The stack item to copy value from.
     */
    virtual void SetValue(std::shared_ptr<StackItem> /* other */) {}

    /**
     * @brief Creates an interop interface wrapper for an object.
     * @param value The object to wrap.
     * @return A new stack item representing the interop interface.
     */
    static std::shared_ptr<StackItem> CreateInteropInterface(void* value);

    /**
     * @brief Creates an interop interface wrapper for a typed object.
     * @tparam T The type of the object.
     * @param value The object to wrap.
     * @return A new stack item representing the interop interface.
     */
    template <typename T>
    static std::shared_ptr<StackItem> CreateInteropInterface(std::shared_ptr<T> value)
    {
        return CreateInteropInterface(static_cast<void*>(value.get()));
    }
};
}  // namespace neo::vm
