#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/byte_vector.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <memory>
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
        template<typename T>
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
                
                // Deserialize using FromStackItem if available, otherwise use binary deserialization
                // For now, create a default object
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
        template<typename T>
        void SetInteroperable(std::shared_ptr<T> obj)
        {
            interoperable_obj_ = std::static_pointer_cast<void>(obj);
            
            // Serialize the object to value_
            if (obj)
            {
                std::stringstream ss;
                io::BinaryWriter writer(ss);
                
                // For IInteroperable objects, we would use ToStackItem then serialize
                // For now, just store a placeholder
                value_ = io::ByteVector(1); // Placeholder
            }
            else
            {
                value_.clear();
            }
        }

    private:
        io::ByteVector value_;
        mutable std::shared_ptr<void> interoperable_obj_;
    };
}
