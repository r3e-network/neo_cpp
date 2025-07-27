#pragma once

#include <neo/json/jtoken.h>

namespace neo::json
{
/**
 * @brief Represents a JSON number value.
 */
class JNumber : public JToken
{
  public:
    /**
     * @brief Constructor.
     * @param value The numeric value.
     */
    explicit JNumber(double value);

    /**
     * @brief Gets the type of this token.
     * @return JTokenType::Number.
     */
    JTokenType GetType() const override;

    /**
     * @brief Converts the current JSON token to a floating point number.
     * @return The numeric value.
     */
    double AsNumber() const override;

    /**
     * @brief Converts the current JSON token to a floating point number.
     * @return The numeric value.
     */
    double GetNumber() const override;

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
     * @brief Implicit conversion to double.
     * @return The numeric value.
     */
    operator double() const override;

    /**
     * @brief Implicit conversion to int.
     * @return The numeric value as int.
     */
    operator int() const override;

    /**
     * @brief Gets the numeric value.
     * @return The numeric value.
     */
    double GetValue() const;

  protected:
    /**
     * @brief Writes this token to JSON output.
     * @param output The output string.
     * @param indented Whether to use indented formatting.
     * @param indent_level The indentation level.
     */
    void WriteJson(std::string& output, bool indented = false, int indent_level = 0) const override;

  private:
    double value_;
};
}  // namespace neo::json
