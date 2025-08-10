#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace neo::io
{
/**
 * @brief Interface for objects that can be serialized/deserialized to/from JSON.
 */
class JsonSerializable
{
   public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~JsonSerializable() = default;

    /**
     * @brief Serializes the object to a JSON value.
     * @return The JSON representation of the object.
     */
    virtual nlohmann::json ToJson() const = 0;

    /**
     * @brief Deserializes the object from a JSON value.
     * @param json The JSON representation of the object.
     */
    virtual void FromJson(const nlohmann::json& json) = 0;

    /**
     * @brief Serializes the object to a JSON string.
     * @param pretty Whether to format the JSON string with indentation.
     * @return The JSON string representation of the object.
     */
    std::string ToJsonString(bool pretty = false) const
    {
        nlohmann::json json = ToJson();
        return pretty ? json.dump(4) : json.dump();
    }

    /**
     * @brief Deserializes the object from a JSON string.
     * @param json The JSON string representation of the object.
     */
    void FromJsonString(const std::string& json) { FromJson(nlohmann::json::parse(json)); }
};
}  // namespace neo::io
