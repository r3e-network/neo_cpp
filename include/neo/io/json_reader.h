#pragma once

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/io/fixed8.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <optional>

namespace neo::io
{
    class IJsonSerializable;

    /**
     * @brief Reads primitive types and complex objects from a JSON value.
     */
    class JsonReader
    {
    public:
        /**
         * @brief Constructs a JsonReader that reads from the specified JSON value.
         * @param json The JSON value to read from.
         */
        explicit JsonReader(const nlohmann::json& json);

        /**
         * @brief Reads a boolean value from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        bool ReadBool(const std::string& key, bool defaultValue = false) const;

        /**
         * @brief Reads an 8-bit unsigned integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        uint8_t ReadUInt8(const std::string& key, uint8_t defaultValue = 0) const;

        /**
         * @brief Reads a 16-bit unsigned integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        uint16_t ReadUInt16(const std::string& key, uint16_t defaultValue = 0) const;

        /**
         * @brief Reads a 32-bit unsigned integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        uint32_t ReadUInt32(const std::string& key, uint32_t defaultValue = 0) const;

        /**
         * @brief Reads a 64-bit unsigned integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        uint64_t ReadUInt64(const std::string& key, uint64_t defaultValue = 0) const;

        /**
         * @brief Reads an 8-bit signed integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        int8_t ReadInt8(const std::string& key, int8_t defaultValue = 0) const;

        /**
         * @brief Reads a 16-bit signed integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        int16_t ReadInt16(const std::string& key, int16_t defaultValue = 0) const;

        /**
         * @brief Reads a 32-bit signed integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        int32_t ReadInt32(const std::string& key, int32_t defaultValue = 0) const;

        /**
         * @brief Reads a 64-bit signed integer from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        int64_t ReadInt64(const std::string& key, int64_t defaultValue = 0) const;

        /**
         * @brief Reads a string from the JSON.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        std::string ReadString(const std::string& key, const std::string& defaultValue = "") const;

        /**
         * @brief Reads a base64 string from the JSON and decodes it to a byte array.
         * @param key The key of the value to read.
         * @return The decoded byte array, or an empty byte array if the key is not found.
         */
        ByteVector ReadBase64String(const std::string& key) const;

        /**
         * @brief Reads a number from the JSON as a double.
         * @param key The key of the value to read.
         * @param defaultValue The default value to return if the key is not found.
         * @return The value read, or the default value if the key is not found.
         */
        double ReadNumber(const std::string& key, double defaultValue = 0.0) const;

        /**
         * @brief Reads a byte array from the JSON.
         * @param key The key of the value to read.
         * @return The value read, or an empty byte array if the key is not found.
         */
        ByteVector ReadBytes(const std::string& key) const;

        /**
         * @brief Reads a UInt160 from the JSON.
         * @param key The key of the value to read.
         * @return The value read, or UInt160::Zero() if the key is not found.
         */
        UInt160 ReadUInt160(const std::string& key) const;

        /**
         * @brief Reads a UInt256 from the JSON.
         * @param key The key of the value to read.
         * @return The value read, or UInt256::Zero() if the key is not found.
         */
        UInt256 ReadUInt256(const std::string& key) const;

        /**
         * @brief Reads a Fixed8 from the JSON.
         * @param key The key of the value to read.
         * @return The value read, or Fixed8::Zero() if the key is not found.
         */
        Fixed8 ReadFixed8(const std::string& key) const;

        /**
         * @brief Reads a JSON object from the JSON.
         * @param key The key of the value to read.
         * @return The value read, or an empty JSON object if the key is not found.
         */
        nlohmann::json ReadObject(const std::string& key) const;

        /**
         * @brief Reads a JSON array from the JSON.
         * @param key The key of the value to read.
         * @return The value read, or an empty JSON array if the key is not found.
         */
        nlohmann::json ReadArray(const std::string& key) const;

        /**
         * @brief Reads a serializable object from the JSON.
         * @tparam T The type of the object to read.
         * @param key The key of the value to read.
         * @return The value read, or std::nullopt if the key is not found.
         */
        template <typename T>
        std::optional<T> ReadSerializable(const std::string& key) const
        {
            static_assert(std::is_base_of<IJsonSerializable, T>::value, "T must derive from IJsonSerializable");
            static_assert(std::is_default_constructible<T>::value, "T must be default constructible");

            if (!json_.contains(key) || !json_[key].is_object())
                return std::nullopt;

            T obj;
            JsonReader reader(json_[key]);
            obj.DeserializeJson(reader);
            return obj;
        }

        /**
         * @brief Reads a vector of serializable objects from the JSON.
         * @tparam T The type of the objects in the vector.
         * @param key The key of the value to read.
         * @return The value read, or an empty vector if the key is not found.
         */
        template <typename T>
        std::vector<T> ReadVector(const std::string& key) const
        {
            static_assert(std::is_base_of<IJsonSerializable, T>::value, "T must derive from IJsonSerializable");
            static_assert(std::is_default_constructible<T>::value, "T must be default constructible");

            if (!json_.contains(key) || !json_[key].is_array())
                return {};

            std::vector<T> result;
            for (const auto& item : json_[key])
            {
                T obj;
                JsonReader reader(item);
                obj.DeserializeJson(reader);
                result.push_back(obj);
            }

            return result;
        }

        /**
         * @brief Gets the underlying JSON value.
         * @return The underlying JSON value.
         */
        const nlohmann::json& GetJson() const { return json_; }

        /**
         * @brief Checks if the JSON contains the specified key.
         * @param key The key to check.
         * @return True if the JSON contains the key, false otherwise.
         */
        bool HasKey(const std::string& key) const { return json_.contains(key); }

        /**
         * @brief Reads the start of an object from the JSON.
         */
        void ReadStartObject() const {}

        /**
         * @brief Reads the end of an object from the JSON.
         */
        void ReadEndObject() const {}

    private:
        const nlohmann::json& json_;
    };
}
