#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace neo::json
{
enum class JTokenType
{
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

// Forward declarations
class JArray;
class JObject;

/**
 * @brief Base class for all JSON tokens.
 */
class JToken
{
   public:
    /**
     * @brief Represents a null token.
     */
    static const std::shared_ptr<JToken> Null;

    virtual ~JToken() = default;

    /**
     * @brief Gets the type of this token.
     * @return The token type.
     */
    virtual JTokenType GetType() const = 0;

    /**
     * @brief Gets or sets the child token at the specified index.
     * @param index The zero-based index of the child token to get or set.
     * @return The child token at the specified index.
     * @throws std::runtime_error if not supported.
     */
    virtual std::shared_ptr<JToken> operator[](int index) const
    {
        throw std::runtime_error("Index access not supported");
    }

    /**
     * @brief Gets or sets the properties of the JSON object.
     * @param key The key of the property to get or set.
     * @return The property with the specified name.
     * @throws std::runtime_error if not supported.
     */
    virtual std::shared_ptr<JToken> operator[](const std::string& key) const
    {
        throw std::runtime_error("Key access not supported");
    }

    /**
     * @brief Gets or sets the properties of the JSON object.
     * @param key The key of the property to get or set.
     * @return The property with the specified name.
     * @throws std::runtime_error if not supported.
     */
    virtual std::shared_ptr<JToken> operator[](const char* key) const { return operator[](std::string(key)); }

    /**
     * @brief Converts the current JSON token to a boolean value.
     * @return The converted value.
     */
    virtual bool AsBoolean() const { return true; }

    /**
     * @brief Converts the current JSON token to a floating point number.
     * @return The converted value.
     */
    virtual double AsNumber() const { return std::numeric_limits<double>::quiet_NaN(); }

    /**
     * @brief Converts the current JSON token to a string.
     * @return The converted value.
     */
    virtual std::string AsString() const { return ToString(); }

    /**
     * @brief Converts the current JSON token to a boolean value.
     * @return The converted value.
     * @throws std::invalid_argument if the JSON token is not a boolean.
     */
    virtual bool GetBoolean() const { throw std::invalid_argument("Token is not a boolean"); }

    /**
     * @brief Converts the current JSON token to a 32-bit signed integer.
     * @return The converted value.
     * @throws std::invalid_argument if the JSON token is not a number.
     * @throws std::overflow_error if the JSON token cannot be converted to a 32-bit signed integer.
     */
    int GetInt32() const
    {
        double d = GetNumber();
        if (std::fmod(d, 1.0) != 0.0) throw std::invalid_argument("Number is not an integer");
        if (d < std::numeric_limits<int>::min() || d > std::numeric_limits<int>::max())
            throw std::overflow_error("Number is out of range for int32");
        return static_cast<int>(d);
    }

    /**
     * @brief Converts the current JSON token to a floating point number.
     * @return The converted value.
     * @throws std::invalid_argument if the JSON token is not a number.
     */
    virtual double GetNumber() const { throw std::invalid_argument("Token is not a number"); }

    /**
     * @brief Converts the current JSON token to a string.
     * @return The converted value.
     * @throws std::invalid_argument if the JSON token is not a string.
     */
    virtual std::string GetString() const { throw std::invalid_argument("Token is not a string"); }

    /**
     * @brief Parses a JSON string into a JToken.
     * @param json The JSON string to parse.
     * @param max_nest The maximum nesting depth when parsing the JSON token.
     * @return The parsed JToken.
     */
    static std::shared_ptr<JToken> Parse(const std::string& json, int max_nest = 64);

   private:
    /**
     * @brief Parses a nlohmann::json value into a JToken.
     * @param j The nlohmann::json value.
     * @return The parsed JToken.
     */
    static std::shared_ptr<JToken> ParseJsonValue(const nlohmann::json& j);

   public:
    /**
     * @brief Converts this token to a string representation.
     * @return The string representation.
     */
    virtual std::string ToString() const = 0;

    /**
     * @brief Converts this token to a string representation.
     * @param indented Whether to use indented formatting.
     * @return The string representation.
     */
    virtual std::string ToString(bool indented) const;

    /**
     * @brief Clones this token.
     * @return A clone of this token.
     */
    virtual std::shared_ptr<JToken> Clone() const = 0;

    /**
     * @brief Checks if this token equals another token.
     * @param other The other token.
     * @return True if equal, false otherwise.
     */
    virtual bool Equals(const JToken& other) const = 0;

    /**
     * @brief Implicit conversion to bool.
     * @return True if the token represents a truthy value.
     */
    virtual operator bool() const;

    /**
     * @brief Implicit conversion to string.
     * @return The string representation of the token.
     */
    virtual operator std::string() const;

    /**
     * @brief Implicit conversion to int.
     * @return The integer representation of the token.
     */
    virtual operator int() const;

    /**
     * @brief Implicit conversion to double.
     * @return The double representation of the token.
     */
    virtual operator double() const;

    /**
     * @brief Writes this token to a JSON writer.
     * @param writer The JSON writer.
     */
    virtual void WriteJson(std::string& output, bool indented = false, int indent_level = 0) const = 0;

   protected:
    /**
     * @brief Helper function to add indentation.
     * @param output The output string.
     * @param indent_level The indentation level.
     */
    static void AddIndentation(std::string& output, int indent_level);
};
}  // namespace neo::json
