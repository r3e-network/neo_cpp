#pragma once

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace neo::cli
{

/**
 * @brief RAII-safe type converter that returns smart pointers instead of raw pointers
 */
class SafeTypeConverters
{
   public:
    /**
     * @brief Type converter function that returns std::any for type safety
     */
    using SafeTypeConverter = std::function<std::any(const std::vector<std::string>&, bool)>;

    /**
     * @brief Gets the singleton instance
     * @return The instance
     */
    static SafeTypeConverters& Instance()
    {
        static SafeTypeConverters instance;
        return instance;
    }

    /**
     * @brief Register a type converter
     * @param typeName The type name
     * @param converter The converter function
     */
    template <typename T>
    void RegisterConverter(const std::string& typeName,
                           std::function<T(const std::vector<std::string>&, bool)> converter)
    {
        converters_[typeName] = [converter](const std::vector<std::string>& args, bool canConsumeAll) -> std::any
        { return std::make_shared<T>(converter(args, canConsumeAll)); };
    }

    /**
     * @brief Get a converted value as smart pointer
     * @param typeName The type name
     * @param args The arguments
     * @param canConsumeAll Whether all args can be consumed
     * @return Smart pointer to converted object
     */
    template <typename T>
    std::shared_ptr<T> Convert(const std::string& typeName, const std::vector<std::string>& args,
                               bool canConsumeAll = false)
    {
        auto it = converters_.find(typeName);
        if (it == converters_.end())
        {
            throw std::runtime_error("No converter found for type: " + typeName);
        }

        try
        {
            std::any result = it->second(args, canConsumeAll);
            return std::any_cast<std::shared_ptr<T>>(result);
        }
        catch (const std::bad_any_cast& e)
        {
            throw std::runtime_error("Type mismatch in converter for " + typeName + ": " + e.what());
        }
    }

    /**
     * @brief Check if converter exists
     * @param typeName The type name
     * @return true if exists
     */
    bool HasConverter(const std::string& typeName) const { return converters_.find(typeName) != converters_.end(); }

    /**
     * @brief Initialize default converters
     */
    void InitializeDefaultConverters();

   private:
    std::unordered_map<std::string, SafeTypeConverter> converters_;

    SafeTypeConverters() = default;
};

}  // namespace neo::cli