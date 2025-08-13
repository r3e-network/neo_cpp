/**
 * @file console_command_attribute.h
 * @brief Console Command Attribute
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace neo::cli
{
/**
 * @brief Attribute for console commands.
 */
class ConsoleCommandAttribute
{
   public:
    /**
     * @brief The command name.
     */
    std::string Name;

    /**
     * @brief The command category.
     */
    std::string Category;

    /**
     * @brief The command description.
     */
    std::string Description;

    /**
     * @brief Constructs a ConsoleCommandAttribute.
     * @param name The command name.
     * @param category The command category.
     * @param description The command description.
     */
    ConsoleCommandAttribute(const std::string& name, const std::string& category = "",
                            const std::string& description = "")
        : Name(name), Category(category), Description(description)
    {
    }

    /**
     * @brief Gets the verbs from the command name.
     * @return The verbs.
     */
    std::vector<std::string> GetVerbs() const
    {
        std::vector<std::string> verbs;
        std::istringstream iss(Name);
        std::string verb;
        while (iss >> verb)
        {
            verbs.push_back(verb);
        }
        return verbs;
    }
};

/**
 * @brief Macro for defining console commands.
 */
#define CONSOLE_COMMAND(name, category, description)                             \
    static ConsoleCommandAttribute name##_command(#name, category, description); \
    void name
}  // namespace neo::cli
