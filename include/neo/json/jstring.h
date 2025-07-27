#pragma once

#include <neo/json/jtoken.h>

namespace neo::json
{
/**
 * @brief Represents a JSON string value.
 */
class JString : public JToken
{
  public:
    /**
     * @brief Constructor.
     * @param value The string value.
     */
    explicit JString(const std::string& value);

    /**
     * @brief Constructor.
     * @param value The string value.
     */
    explicit JString(std::string&& value);

    /**
     * @brief Gets the type of this token.
     * @return JTokenType::String.
     */
    JTokenType GetType() const override;

    /**
     * @brief Converts the current JSON token to a string.
     * @return The string value.
     */
    std::string AsString() const override;

    /**
     * @brief Converts the current JSON token to a string.
     * @return The string value.
     */
    std::string GetString() const override;

    /**
     * @brief Converts this token to a string representation.
     * @return The JSON-encoded string representation.
     */
    std::string ToString() const override;

    /**
     * @brief Clones this token.
     * @return A clone of this token.
     */
    std::shared_ptr<JToken> Clone() const override;

    /**
     * @brief Checks if this token equals another token.
     * @param other The other token.
     * @return True if equal, false otherwise.
     */
    bool Equals(const JToken& other) const override;

    /**
     * @brief Implicit conversion to string.
     * @return The string value.
     */
    operator std::string() const override;

    /**
     * @brief Gets the string value.
     * @return The string value.
     */
    const std::string& GetValue() const;

  protected:
    /**
     * @brief Writes this token to JSON output.
     * @param output The output string.
     * @param indented Whether to use indented formatting.
     * @param indent_level The indentation level.
     */
    void WriteJson(std::string& output, bool indented = false, int indent_level = 0) const override;

  private:
    std::string value_;

    /**
     * @brief Escapes a string for JSON output.
     * @param str The string to escape.
     * @return The escaped string.
     */
    static std::string EscapeString(const std::string& str);
};
}  // namespace neo::json
