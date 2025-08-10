#pragma once

#include <neo/io/byte_span.h>
#include <neo/io/byte_vector.h>
#include <neo/io/fixed8.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace neo::io
{
class IJsonSerializable;

/**
 * @brief Writes primitive types and complex objects to a JSON value.
 */
class JsonWriter
{
   public:
    /**
     * @brief Constructs a JsonWriter that writes to a new JSON object.
     */
    JsonWriter();

    /**
     * @brief Constructs a JsonWriter that writes to the specified JSON value.
     * @param json The JSON value to write to.
     */
    explicit JsonWriter(nlohmann::json& json);

    /**
     * @brief Writes a boolean value to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, bool value);

    /**
     * @brief Writes an 8-bit unsigned integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, uint8_t value);

    /**
     * @brief Writes a 16-bit unsigned integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, uint16_t value);

    /**
     * @brief Writes a 32-bit unsigned integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, uint32_t value);

    /**
     * @brief Writes a 64-bit unsigned integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, uint64_t value);

    /**
     * @brief Writes an 8-bit signed integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, int8_t value);

    /**
     * @brief Writes a 16-bit signed integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, int16_t value);

    /**
     * @brief Writes a 32-bit signed integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, int32_t value);

    /**
     * @brief Writes a 64-bit signed integer to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, int64_t value);

    /**
     * @brief Writes a string to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const std::string& value);

    /**
     * @brief Writes a base64 encoded byte array to the JSON.
     * @param key The key to write the value to.
     * @param value The byte array to encode and write.
     */
    void WriteBase64String(const std::string& key, const ByteSpan& value);

    /**
     * @brief Writes a string value to the JSON.
     * @param key The key to write the value to.
     * @param value The string value to write.
     */
    void WriteString(const std::string& key, const std::string& value);

    /**
     * @brief Writes a numeric value to the JSON.
     * @param key The key to write the value to.
     * @param value The numeric value to write.
     */
    void WriteNumber(const std::string& key, double value);

    /**
     * @brief Writes a byte array to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const ByteSpan& value);

    /**
     * @brief Writes a UInt160 to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const UInt160& value);

    /**
     * @brief Writes a UInt256 to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const UInt256& value);

    /**
     * @brief Writes a Fixed8 to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const Fixed8& value);

    /**
     * @brief Writes a JSON object to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const nlohmann::json& value);

    /**
     * @brief Writes a serializable object to the JSON.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    void Write(const std::string& key, const IJsonSerializable& value);

    /**
     * @brief Writes a vector of serializable objects to the JSON.
     * @tparam T The type of the objects in the vector.
     * @param key The key to write the value to.
     * @param value The value to write.
     */
    template <typename T>
    void WriteVector(const std::string& key, const std::vector<T>& value)
    {
        static_assert(std::is_base_of<IJsonSerializable, T>::value, "T must derive from IJsonSerializable");

        nlohmann::json array = nlohmann::json::array();
        for (const auto& item : value)
        {
            nlohmann::json itemJson = nlohmann::json::object();
            JsonWriter writer(itemJson);
            item.SerializeJson(writer);
            array.push_back(itemJson);
        }

        json_[key] = array;
    }

    /**
     * @brief Gets the underlying JSON value.
     * @return The underlying JSON value.
     */
    const nlohmann::json& GetJson() const { return json_; }

    /**
     * @brief Writes a property to the JSON.
     * @param name The name of the property.
     * @param value The value of the property.
     */
    template <typename T>
    void WriteProperty(const std::string& name, const T& value)
    {
        json_[name] = value;
    }

    /**
     * @brief Writes a property name to the JSON.
     * @param name The name of the property.
     */
    void WritePropertyName(const std::string& name) { currentPropertyName_ = name; }

    /**
     * @brief Writes the start of an array to the JSON.
     */
    void WriteStartArray()
    {
        if (!currentPropertyName_.empty())
        {
            json_[currentPropertyName_] = nlohmann::json::array();
            currentArray_ = &json_[currentPropertyName_];
            currentPropertyName_.clear();
        }
        else
        {
            json_ = nlohmann::json::array();
            currentArray_ = &json_;
        }
    }

    /**
     * @brief Writes the end of an array to the JSON.
     */
    void WriteEndArray() { currentArray_ = nullptr; }

    /**
     * @brief Writes the start of an object to the JSON.
     */
    void WriteStartObject()
    {
        if (currentArray_ != nullptr)
        {
            currentArray_->push_back(nlohmann::json::object());
            currentObject_ = &currentArray_->back();
        }
        else if (!currentPropertyName_.empty())
        {
            json_[currentPropertyName_] = nlohmann::json::object();
            currentObject_ = &json_[currentPropertyName_];
            currentPropertyName_.clear();
        }
        else
        {
            json_ = nlohmann::json::object();
            currentObject_ = &json_;
        }
    }

    /**
     * @brief Writes the end of an object to the JSON.
     */
    void WriteEndObject() { currentObject_ = nullptr; }

    /**
     * @brief Writes an array to the JSON.
     * @tparam T The type of the elements in the array.
     * @param key The key to write the array to.
     * @param value The array to write.
     * @param writer The function to write each element.
     */
    template <typename T>
    void WriteArray(const std::string& key, const std::vector<T>& value,
                    std::function<void(JsonWriter&, const T&)> writer)
    {
        nlohmann::json array = nlohmann::json::array();
        for (const auto& item : value)
        {
            nlohmann::json itemJson = nlohmann::json::object();
            JsonWriter itemWriter(itemJson);
            writer(itemWriter, item);
            array.push_back(itemJson);
        }
        json_[key] = array;
    }

    /**
     * @brief Writes a string value to the JSON for streaming writing.
     * @param value The string value to write.
     */
    void WriteString(const std::string& value);

    /**
     * @brief Writes a numeric value to the JSON for streaming writing.
     * @param value The numeric value to write.
     */
    void WriteNumber(double value);

    /**
     * @brief Writes an integer value to the JSON for streaming writing.
     * @param value The integer value to write.
     */
    void WriteNumber(int value);

   private:
    nlohmann::json& json_;
    nlohmann::json ownedJson_;
    std::string currentPropertyName_;
    nlohmann::json* currentArray_ = nullptr;
    nlohmann::json* currentObject_ = nullptr;
};
}  // namespace neo::io
