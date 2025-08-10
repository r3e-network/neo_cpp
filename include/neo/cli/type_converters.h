#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::cli
{
/**
 * @brief Type converter function.
 * @warning Returns raw pointer - caller MUST take ownership and delete.
 *          Consider using SafeTypeConverters for RAII safety.
 */
using TypeConverter = std::function<void*(const std::vector<std::string>&, bool)>;

/**
 * @brief Manages type converters.
 */
class TypeConverters
{
   public:
    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static TypeConverters& Instance();

    /**
     * @brief Registers a type converter.
     * @param typeName The type name.
     * @param converter The converter.
     */
    void RegisterConverter(const std::string& typeName, const TypeConverter& converter);

    /**
     * @brief Gets a type converter.
     * @param typeName The type name.
     * @return The converter.
     */
    TypeConverter GetConverter(const std::string& typeName) const;

    /**
     * @brief Checks if a type converter exists.
     * @param typeName The type name.
     * @return True if the converter exists, false otherwise.
     */
    bool HasConverter(const std::string& typeName) const;

    /**
     * @brief Gets all type converters.
     * @return All type converters.
     */
    const std::unordered_map<std::string, TypeConverter>& GetAllConverters() const;

    /**
     * @brief Initializes the default type converters.
     */
    void InitializeDefaultConverters();

   private:
    std::unordered_map<std::string, TypeConverter> converters_;

    TypeConverters();
};
}  // namespace neo::cli
