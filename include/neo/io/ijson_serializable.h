#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace neo::io
{
    class JsonWriter;
    class JsonReader;

    /**
     * @brief Interface for objects that can be serialized/deserialized to/from JSON.
     */
    class IJsonSerializable
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~IJsonSerializable() = default;

        /**
         * @brief Serializes the object to a JSON writer.
         * @param writer The JSON writer.
         */
        virtual void SerializeJson(JsonWriter& writer) const = 0;

        /**
         * @brief Deserializes the object from a JSON reader.
         * @param reader The JSON reader.
         */
        virtual void DeserializeJson(const JsonReader& reader) = 0;

        /**
         * @brief Serializes the object to a JSON object.
         * @return The serialized object.
         */
        nlohmann::json ToJson() const;

        /**
         * @brief Deserializes an object from a JSON object.
         * @tparam T The type of the object to deserialize.
         * @param json The JSON object.
         * @return The deserialized object.
         */
        template <typename T>
        static T FromJson(const nlohmann::json& json)
        {
            static_assert(std::is_base_of<IJsonSerializable, T>::value, "T must derive from IJsonSerializable");
            static_assert(std::is_default_constructible<T>::value, "T must be default constructible");
            
            T obj;
            obj.DeserializeFromJson(json);
            return obj;
        }

        /**
         * @brief Deserializes an object from a JSON object.
         * @param json The JSON object.
         */
        void DeserializeFromJson(const nlohmann::json& json);

        /**
         * @brief Serializes the object to a JSON string.
         * @param pretty Whether to pretty-print the JSON.
         * @return The serialized object.
         */
        std::string ToJsonString(bool pretty = false) const;

        /**
         * @brief Deserializes an object from a JSON string.
         * @tparam T The type of the object to deserialize.
         * @param json The JSON string.
         * @return The deserialized object.
         */
        template <typename T>
        static T FromJsonString(const std::string& json)
        {
            static_assert(std::is_base_of<IJsonSerializable, T>::value, "T must derive from IJsonSerializable");
            static_assert(std::is_default_constructible<T>::value, "T must be default constructible");
            
            T obj;
            obj.DeserializeFromJsonString(json);
            return obj;
        }

        /**
         * @brief Deserializes an object from a JSON string.
         * @param json The JSON string.
         */
        void DeserializeFromJsonString(const std::string& json);
    };
}
