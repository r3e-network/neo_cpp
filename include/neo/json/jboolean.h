#pragma once

#include <neo/json/jtoken.h>

namespace neo::json
{
/**
 * @brief Represents a JSON boolean value.
 */
class JBoolean : public JToken
{
  public:
    /**
     * @brief Constructor.
     * @param value The boolean value.
     */
    explicit JBoolean(bool value);

    /**
     * @brief Gets the type of this token.
     * @return JTokenType::Boolean.
     */
    JTokenType GetType() const override;

    /**
     * @brief Converts the current JSON token to a boolean value.
     * @return The boolean value.
     */
    bool AsBoolean() const override;

    /**
     * @brief Converts the current JSON token to a boolean value.
     * @return The boolean value.
     */
    bool GetBoolean() const override;

    /**
     * @brief Converts this token to a string representation.
     * @return The string representation.
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
     * @brief Implicit conversion to bool.
     * @return The boolean value.
     */
    operator bool() const override;

    /**
     * @brief Gets the boolean value.
     * @return The boolean value.
     */
    bool GetValue() const;

    /**
     * @brief Writes this token to JSON output.
     * @param output The output string.
     * @param indented Whether to use indented formatting.
     * @param indent_level The indentation level.
     */
    void WriteJson(std::string& output, bool indented = false, int indent_level = 0) const override;

  protected:
  private:
    bool value_;
};
}  // namespace neo::json
