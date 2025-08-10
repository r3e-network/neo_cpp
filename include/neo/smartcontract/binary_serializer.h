#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/byte_vector.h>
#include <neo/vm/stack_item.h>

#include <memory>

namespace neo::smartcontract
{
/**
 * @brief A binary serializer for StackItem objects.
 * This class provides functionality to serialize and deserialize StackItem objects
 * to and from binary format, exactly matching the C# BinarySerializer implementation.
 */
class BinarySerializer
{
   public:
    /**
     * @brief Serializes a StackItem to a byte array.
     * @param item The StackItem to serialize.
     * @param maxSize The maximum size of the result.
     * @param maxItems The maximum number of items to serialize.
     * @return The serialized byte array.
     */
    static io::ByteVector Serialize(std::shared_ptr<vm::StackItem> item, int64_t maxSize = 2048,
                                    int64_t maxItems = 2048);

    /**
     * @brief Deserializes a StackItem from a byte array.
     * @param data The byte array to deserialize.
     * @param maxSize The maximum size allowed.
     * @param maxItems The maximum number of items allowed.
     * @return The deserialized StackItem.
     */
    static std::shared_ptr<vm::StackItem> Deserialize(const io::ByteSpan& data, int64_t maxSize = 2048,
                                                      int64_t maxItems = 2048);

   private:
    /**
     * @brief Serializes a StackItem to a BinaryWriter.
     * @param writer The BinaryWriter to write to.
     * @param item The StackItem to serialize.
     * @param maxSize The maximum size allowed.
     * @param maxItems The maximum number of items allowed.
     * @param itemCount Reference to the current item count.
     */
    static void SerializeStackItem(io::BinaryWriter& writer, std::shared_ptr<vm::StackItem> item, int64_t maxSize,
                                   int64_t maxItems, int64_t& itemCount);

    /**
     * @brief Deserializes a StackItem from a BinaryReader.
     * @param reader The BinaryReader to read from.
     * @param maxSize The maximum size allowed.
     * @param maxItems The maximum number of items allowed.
     * @param itemCount Reference to the current item count.
     * @return The deserialized StackItem.
     */
    static std::shared_ptr<vm::StackItem> DeserializeStackItem(io::BinaryReader& reader, int64_t maxSize,
                                                               int64_t maxItems, int64_t& itemCount);
};
}  // namespace neo::smartcontract
