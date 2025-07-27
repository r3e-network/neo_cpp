#pragma once

#include <memory>
#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>

namespace neo::io
{
class BinaryWriter;
class BinaryReader;

/**
 * @brief Interface for objects that can be serialized/deserialized.
 */
class ISerializable
{
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~ISerializable() = default;

    /**
     * @brief Serializes the object to a binary writer.
     * @param writer The binary writer.
     */
    virtual void Serialize(BinaryWriter& writer) const = 0;

    /**
     * @brief Deserializes the object from a binary reader.
     * @param reader The binary reader.
     */
    virtual void Deserialize(BinaryReader& reader) = 0;

    /**
     * @brief Serializes the object to a byte vector.
     * @return The serialized object.
     */
    ByteVector ToArray() const;

    /**
     * @brief Deserializes an object from a byte span.
     * @tparam T The type of the object to deserialize.
     * @param data The serialized data.
     * @return The deserialized object.
     */
    template <typename T>
    static T FromArray(const ByteSpan& data)
    {
        static_assert(std::is_base_of<ISerializable, T>::value, "T must derive from ISerializable");
        static_assert(std::is_default_constructible<T>::value, "T must be default constructible");

        T obj;
        obj.DeserializeFromArray(data);
        return obj;
    }

    /**
     * @brief Deserializes an object from a byte span.
     * @param data The serialized data.
     */
    void DeserializeFromArray(const ByteSpan& data);
};
}  // namespace neo::io
