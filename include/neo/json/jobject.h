#pragma once

#include <neo/json/jtoken.h>
#include <neo/json/ordered_dictionary.h>
#include <string>

namespace neo::json
{
/**
 * @brief Represents a JSON object.
 */
class JObject : public JToken
{
  public:
    using Properties = OrderedDictionary<std::string, std::shared_ptr<JToken>>;

    /**
     * @brief Default constructor.
     */
    JObject();

    /**
     * @brief Constructor with initial properties.
     * @param properties The initial properties.
     */
    explicit JObject(const Properties& properties);

    /**
     * @brief Gets the type of this token.
     * @return JTokenType::Object.
     */
    JTokenType GetType() const override;

    /**
     * @brief Gets or sets the properties of the JSON object.
     * @param key The key of the property to get or set.
     * @return The property with the specified name.
     */
    std::shared_ptr<JToken> operator[](const std::string& key) const override;

    /**
     * @brief Gets or sets the properties of the JSON object.
     * @param key The key of the property to get or set.
     * @return The property with the specified name.
     */
    std::shared_ptr<JToken> operator[](const char* key) const;

    /**
     * @brief Converts this object to a string representation.
     * @return The JSON string representation.
     */
    std::string ToString() const override;

    /**
     * @brief Clones this object.
     * @return A clone of this object.
     */
    std::shared_ptr<JToken> Clone() const override;

    /**
     * @brief Checks if this object equals another token.
     * @param other The other token.
     * @return True if equal, false otherwise.
     */
    bool Equals(const JToken& other) const override;

    /**
     * @brief Sets a property.
     * @param key The property key.
     * @param value The property value.
     */
    void SetProperty(const std::string& key, std::shared_ptr<JToken> value);

    /**
     * @brief Gets the properties.
     * @return The properties.
     */
    const Properties& GetProperties() const;

    /**
     * @brief Gets the properties (mutable).
     * @return The properties.
     */
    Properties& GetProperties();

    /**
     * @brief Determines whether the JSON object contains a property with the specified name.
     * @param key The property name to locate in the JSON object.
     * @return True if the JSON object contains a property with the name; otherwise, false.
     */
    bool ContainsProperty(const std::string& key) const;

    /**
     * @brief Clears all properties from the object.
     */
    void Clear();

    /**
     * @brief Gets the number of properties.
     * @return The number of properties.
     */
    size_t Count() const;

  protected:
    /**
     * @brief Writes this token to JSON output.
     * @param output The output string.
     * @param indented Whether to use indented formatting.
     * @param indent_level The indentation level.
     */
    void WriteJson(std::string& output, bool indented = false, int indent_level = 0) const override;

  private:
    Properties properties_;
};
}  // namespace neo::json
