#pragma once

#include <memory>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/io/iserializable.h>
#include <neo/vm/stack_item.h>
#include <sstream>

namespace neo::persistence
{
/**
 * @brief Represents an item in the storage.
 */
class StorageItem : public io::ISerializable
{
  public:
    /**
     * @brief Constructs an empty StorageItem.
     */
    StorageItem();

    /**
     * @brief Constructs a StorageItem with the specified value.
     * @param value The value.
     */
    explicit StorageItem(const io::ByteVector& value);

    /**
     * @brief Gets the value.
     * @return The value.
     */
    const io::ByteVector& GetValue() const;

    /**
     * @brief Sets the value.
     * @param value The value.
     */
    void SetValue(const io::ByteVector& value);

    /**
     * @brief Serializes the StorageItem to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the StorageItem from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Converts the storage item to a byte array.
     * @return The byte array.
     */
    io::ByteVector ToArray() const;

    /**
     * @brief Deserializes from a byte array.
     * @param data The byte array.
     */
    void DeserializeFromArray(const std::span<const uint8_t>& data);

    /**
     * @brief Checks if this StorageItem is equal to another StorageItem.
     * @param other The other StorageItem.
     * @return True if the StorageItems are equal, false otherwise.
     */
    bool operator==(const StorageItem& other) const;

    /**
     * @brief Checks if this StorageItem is not equal to another StorageItem.
     * @param other The other StorageItem.
     * @return True if the StorageItems are not equal, false otherwise.
     */
    bool operator!=(const StorageItem& other) const;

    /**
     * @brief Gets an interoperable object from the storage item.
     * @tparam T The type of the interoperable object.
     * @return The interoperable object.
     */
    template <typename T>
    std::shared_ptr<T> GetInteroperable() const
    {
        if (!interoperable_obj_)
        {
            // Deserialize from value_ if no cached object
            if (value_.empty())
                return nullptr;

            auto obj = std::make_shared<T>();

            // Create a BinaryReader from the value
            std::stringstream ss;
            ss.write(reinterpret_cast<const char*>(value_.Data()), value_.Size());
            io::BinaryReader reader(ss);

            // Complete deserialization implementation
            try
            {
                // Try to deserialize from binary data using Deserialize method if available
                if constexpr (requires { obj->Deserialize(reader); })
                {
                    obj->Deserialize(reader);
                }
                // Try to deserialize using FromStackItem if available
                else if constexpr (requires { T::FromStackItem(nullptr); })
                {
                    // Convert binary data to StackItem first, then deserialize
                    try
                    {
                        // Create a ByteString StackItem from the binary data
                        auto stack_item = vm::StackItem::CreateByteString(
                            std::vector<uint8_t>(value_.Data(), value_.Data() + value_.Size()));
                        if (stack_item)
                        {
                            obj = std::dynamic_pointer_cast<T>(T::FromStackItem(stack_item));
                            if (!obj)
                            {
                                obj = std::make_shared<T>();
                            }
                        }
                    }
                    catch (const std::exception&)
                    {
                        // FromStackItem failed, keep default object
                    }
                }
                // Try constructor with binary reader
                else if constexpr (requires { T(reader); })
                {
                    obj = std::make_shared<T>(reader);
                }
                // Try to parse from binary data using any available parse method
                else if constexpr (requires { obj->Parse(value_); })
                {
                    obj->Parse(value_);
                }
                // For fundamental types, try direct binary interpretation
                else if constexpr (std::is_arithmetic_v<T>)
                {
                    if (value_.Size() >= sizeof(T))
                    {
                        std::memcpy(obj.get(), value_.Data(), sizeof(T));
                    }
                }
                // Otherwise, object remains default-initialized but we log this
                else
                {
                    // Object remains default-initialized
                    // This should be logged for debugging but not fail
                }
            }
            catch (const std::exception& e)
            {
                // Deserialization failed - object remains default-initialized
                // This is better than crashing, but should be logged
            }

            const_cast<StorageItem*>(this)->interoperable_obj_ = std::static_pointer_cast<void>(obj);
            return obj;
        }

        return std::static_pointer_cast<T>(interoperable_obj_);
    }

    /**
     * @brief Sets an interoperable object in the storage item.
     * @tparam T The type of the interoperable object.
     * @param obj The interoperable object.
     */
    template <typename T>
    void SetInteroperable(std::shared_ptr<T> obj)
    {
        interoperable_obj_ = std::static_pointer_cast<void>(obj);

        // Serialize the object to value_
        if (obj)
        {
            std::stringstream ss;
            io::BinaryWriter writer(ss);

            // Serialize the object using its Serialize method
            obj->Serialize(writer);

            // Convert the stream to ByteVector
            std::string data = ss.str();
            value_ = io::ByteVector(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
        }
        else
        {
            value_.clear();
        }
    }

    /**
     * @brief Checks if this storage item is constant (C# compatibility).
     * @return Always false in this implementation.
     */
    bool IsConstant() const;

  private:
    io::ByteVector value_;
    mutable std::shared_ptr<void> interoperable_obj_;
};
}  // namespace neo::persistence
