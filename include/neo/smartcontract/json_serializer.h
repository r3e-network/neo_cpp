/**
 * @file json_serializer.h
 * @brief JSON serialization utilities
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/vm/stack_item.h>

#include <memory>
#include <nlohmann/json.hpp>

namespace neo::smartcontract
{
/**
 * @brief A JSON serializer for StackItem objects.
 * This class provides functionality to serialize and deserialize StackItem objects
 * to and from JSON format, exactly matching the C# JsonSerializer implementation.
 */
class JsonSerializer
{
   public:
    /**
     * @brief Serializes a StackItem to a JSON object.
     * @param item The StackItem to serialize.
     * @return The serialized JSON object.
     */
    static nlohmann::json Serialize(std::shared_ptr<vm::StackItem> item);

    /**
     * @brief Serializes a StackItem to a JSON byte array.
     * @param item The StackItem to serialize.
     * @param maxSize The maximum size of the result.
     * @return The serialized JSON as a byte array.
     */
    static io::ByteVector SerializeToByteArray(std::shared_ptr<vm::StackItem> item, int64_t maxSize = 2048);

    /**
     * @brief Deserializes a StackItem from a JSON object.
     * @param json The JSON object to deserialize.
     * @param maxSize The maximum size allowed.
     * @param maxItems The maximum number of items allowed.
     * @return The deserialized StackItem.
     */
    static std::shared_ptr<vm::StackItem> Deserialize(const nlohmann::json& json, int64_t maxSize = 2048,
                                                      int64_t maxItems = 2048);

    /**
     * @brief Deserializes a StackItem from a JSON byte array.
     * @param data The JSON byte array to deserialize.
     * @param maxSize The maximum size allowed.
     * @param maxItems The maximum number of items allowed.
     * @return The deserialized StackItem.
     */
    static std::shared_ptr<vm::StackItem> Deserialize(const io::ByteSpan& data, int64_t maxSize = 2048,
                                                      int64_t maxItems = 2048);

   private:
    /**
     * @brief Serializes a StackItem to a JSON object (internal implementation).
     * @param item The StackItem to serialize.
     * @param itemCount Reference to the current item count.
     * @return The serialized JSON object.
     */
    static nlohmann::json SerializeStackItem(std::shared_ptr<vm::StackItem> item, int64_t& itemCount);

    /**
     * @brief Deserializes a StackItem from a JSON object (internal implementation).
     * @param json The JSON object to deserialize.
     * @param maxSize The maximum size allowed.
     * @param maxItems The maximum number of items allowed.
     * @param itemCount Reference to the current item count.
     * @return The deserialized StackItem.
     */
    static std::shared_ptr<vm::StackItem> DeserializeStackItem(const nlohmann::json& json, int64_t maxSize,
                                                               int64_t maxItems, int64_t& itemCount);

    /**
     * @brief Maximum safe integer value for JSON (2^53 - 1).
     */
    static constexpr int64_t MAX_SAFE_INTEGER = 9007199254740991LL;

    /**
     * @brief Minimum safe integer value for JSON (-(2^53 - 1)).
     */
    static constexpr int64_t MIN_SAFE_INTEGER = -9007199254740991LL;
};
}  // namespace neo::smartcontract
