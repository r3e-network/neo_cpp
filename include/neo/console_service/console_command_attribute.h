#pragma once

#include <string>

namespace neo::console_service
{
    /**
     * @brief Attribute for console commands (placeholder).
     */
    class ConsoleCommandAttribute
    {
    public:
        std::string command;
        std::string category;
        std::string description;
        
        ConsoleCommandAttribute(const std::string& cmd, const std::string& cat = "", const std::string& desc = "")
            : command(cmd), category(cat), description(desc) {}
    };
}
