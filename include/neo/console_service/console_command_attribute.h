#pragma once

#include <string>

namespace neo::console_service
{
/**
 * @brief Attribute for console commands.
 * Stores metadata about a console command including its name, category, and description.
 */
class ConsoleCommandAttribute
{
  public:
    std::string command;
    std::string category;
    std::string description;

    ConsoleCommandAttribute(const std::string& cmd, const std::string& cat = "", const std::string& desc = "")
        : command(cmd), category(cat), description(desc)
    {
    }
};
}  // namespace neo::console_service
