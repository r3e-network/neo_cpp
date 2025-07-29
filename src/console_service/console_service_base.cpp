#include <algorithm>
#include <chrono>
#include <csignal>
#include <iostream>
#include <neo/console_service/console_service_base.h>
#include <neo/core/safe_conversions.h>
#include <sstream>
#include <thread>

namespace neo::console_service
{
// Static instance for signal handling
static ConsoleServiceBase* g_service_instance = nullptr;

ConsoleServiceBase::ConsoleServiceBase() : show_prompt_(true), running_(false)
{
    g_service_instance = this;
    RegisterDefaultHandlers();
}

std::string ConsoleServiceBase::GetDepends() const
{
    return "";
}

std::string ConsoleServiceBase::GetPrompt() const
{
    return "service";
}

bool ConsoleServiceBase::GetShowPrompt() const
{
    return show_prompt_;
}

void ConsoleServiceBase::SetShowPrompt(bool show_prompt)
{
    show_prompt_ = show_prompt;
}

bool ConsoleServiceBase::OnStart(const std::vector<std::string>& args)
{
    (void)args;  // Suppress unused parameter warning
    // Register signal handlers
    std::signal(SIGTERM, SigTermEventHandler);
    std::signal(SIGINT, CancelHandler);
    return true;
}

void ConsoleServiceBase::OnStop()
{
    // Override in derived classes
}

void ConsoleServiceBase::Run(const std::vector<std::string>& args)
{
    // Check for special arguments
    if (args.size() == 1)
    {
        if (args[0] == "/install")
        {
#ifdef _WIN32
            try
            {
                auto result = InstallWindowsService();
                if (result)
                {
                    ConsoleHelper::Info("Service installation", "Neo service installed successfully");
                }
                else
                {
                    ConsoleHelper::Error("Service installation", "Failed to install Neo service");
                }
            }
            catch (const std::exception& e)
            {
                ConsoleHelper::Error("Service installation", e.what());
            }
#else
            ConsoleHelper::Warning("Only support for installing services on Windows.");
#endif
            return;
        }
        else if (args[0] == "/uninstall")
        {
#ifdef _WIN32
            try
            {
                auto result = UninstallWindowsService();
                if (result)
                {
                    ConsoleHelper::Info("Service uninstallation", "Neo service uninstalled successfully");
                }
                else
                {
                    ConsoleHelper::Error("Service uninstallation", "Failed to uninstall Neo service");
                }
            }
            catch (const std::exception& e)
            {
                ConsoleHelper::Error("Service uninstallation", e.what());
            }
#else
            ConsoleHelper::Warning("Only support for uninstalling services on Windows.");
#endif
            return;
        }
    }

    // Start the service
    if (OnStart(args))
    {
        RunConsole();
    }
    OnStop();
}

void ConsoleServiceBase::RegisterCommand(std::shared_ptr<void> instance, const std::string& name)
{
    if (!name.empty())
    {
        std::string lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        instances_[lower_name] = instance;

        // Register the command instance for future method discovery
        // This enables dynamic command registration for console services
        ConsoleHelper::Info("Command", "Registered: " + name);
    }
}

bool ConsoleServiceBase::OnCommand(const std::string& command_line)
{
    if (command_line.empty())
        return true;

    auto command_args = CommandToken::Parse(command_line);

    // Simple command processing - in a full implementation this would be more sophisticated
    auto args = CommandToken::ToArguments(command_args);

    if (args.empty())
        return true;

    std::string command = args[0];
    std::transform(command.begin(), command.end(), command.begin(), ::tolower);

    // Handle built-in commands
    if (command == "help")
    {
        std::string key = args.size() > 1 ? args[1] : "";
        OnHelpCommand(key);
        return true;
    }
    else if (command == "clear")
    {
        OnClear();
        return true;
    }
    else if (command == "version")
    {
        OnVersion();
        return true;
    }
    else if (command == "exit")
    {
        OnExit();
        return true;
    }

    // Command not found
    ConsoleHelper::Error("Command not found: " + command);
    return false;
}

void ConsoleServiceBase::OnHelpCommand(const std::string& key)
{
    if (key.empty() || key == "help")
    {
        std::cout << "Base Commands:" << std::endl;
        std::cout << "\thelp [command]" << std::endl;
        std::cout << "\tclear" << std::endl;
        std::cout << "\tversion" << std::endl;
        std::cout << "\texit" << std::endl;
    }
    else
    {
        if (key == "help")
        {
            std::cout << "Shows help information for commands." << std::endl;
            std::cout << "You can call this command like this:" << std::endl;
            std::cout << "\thelp [command]" << std::endl;
        }
        else if (key == "clear")
        {
            std::cout << "Clear is used in order to clean the console output." << std::endl;
            std::cout << "You can call this command like this:" << std::endl;
            std::cout << "\tclear" << std::endl;
        }
        else if (key == "version")
        {
            std::cout << "Show the current version." << std::endl;
            std::cout << "You can call this command like this:" << std::endl;
            std::cout << "\tversion" << std::endl;
        }
        else if (key == "exit")
        {
            std::cout << "Exit the node." << std::endl;
            std::cout << "You can call this command like this:" << std::endl;
            std::cout << "\texit" << std::endl;
        }
        else
        {
            ConsoleHelper::Error("Command not found: " + key);
        }
    }
}

void ConsoleServiceBase::OnClear()
{
    ConsoleHelper::Clear();
}

void ConsoleServiceBase::OnVersion()
{
    std::cout << "Neo C++ Node v1.0.0" << std::endl;
}

void ConsoleServiceBase::OnExit()
{
    running_ = false;
}

void ConsoleServiceBase::RunConsole()
{
    running_ = true;

    while (running_)
    {
        std::string command = ReadTask();
        if (command.empty() && !running_)
            break;

        if (!command.empty())
        {
            // Add to history
            if (command_history_.size() >= HistorySize)
            {
                command_history_.erase(command_history_.begin());
            }
            command_history_.push_back(command);

            // Process command
            try
            {
                OnCommand(command);
            }
            catch (const std::exception& ex)
            {
                ConsoleHelper::Error(ex.what());
            }
        }
    }
}

std::string ConsoleServiceBase::ReadTask()
{
    if (show_prompt_)
    {
        ConsoleHelper::SetForegroundColor(ConsoleColor::Green);
        std::cout << GetPrompt() << "> ";
        ConsoleHelper::ResetColor();
    }

    std::string input;
    std::getline(std::cin, input);

    // Handle Ctrl+C or EOF
    if (std::cin.eof() || !running_)
    {
        return "";
    }

    return input;
}

bool ConsoleServiceBase::TryProcessValue(const std::type_info& parameter_type,
                                         std::vector<std::shared_ptr<CommandToken>>& args, bool can_consume_all,
                                         void*& value)
{
    auto it = handlers_.find(&parameter_type);
    if (it != handlers_.end())
    {
        value = it->second(args, can_consume_all);
        return true;
    }

    value = nullptr;
    return false;
}

void ConsoleServiceBase::TriggerGracefulShutdown()
{
    if (running_)
    {
        running_ = false;
    }
}

void ConsoleServiceBase::SigTermEventHandler(int signal)
{
    (void)signal;  // Suppress unused parameter warning
    if (g_service_instance)
    {
        g_service_instance->TriggerGracefulShutdown();
    }
}

void ConsoleServiceBase::CancelHandler(int signal)
{
    (void)signal;  // Suppress unused parameter warning
    if (g_service_instance)
    {
        g_service_instance->TriggerGracefulShutdown();
    }
}

void ConsoleServiceBase::RegisterDefaultHandlers()
{
    // Register string handler
    RegisterCommandHandler<std::string>(
        [](std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all) -> std::string
        { return CommandToken::ReadString(args, consume_all); });

    // Register int handler with safe conversion
    RegisterCommandHandler<int>(
        [](std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all) -> int
        {
            (void)consume_all;  // Suppress unused parameter warning
            std::string str = CommandToken::ReadString(args, false);
            return neo::core::SafeConversions::SafeToInt32(str);
        });

    // Register bool handler
    RegisterCommandHandler<bool>(
        [](std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all) -> bool
        {
            (void)consume_all;  // Suppress unused parameter warning
            std::string str = CommandToken::ReadString(args, false);
            std::transform(str.begin(), str.end(), str.begin(), ::tolower);
            return str == "1" || str == "yes" || str == "y" || str == "true";
        });
}

#ifdef _WIN32
#include <windows.h>

bool ConsoleServiceBase::InstallWindowsService()
{
    SC_HANDLE sc_manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!sc_manager)
    {
        return false;
    }

    // Get the path to the current executable
    char exe_path[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, exe_path, MAX_PATH))
    {
        CloseServiceHandle(sc_manager);
        return false;
    }

    std::string service_name = GetServiceName();
    std::string display_name = service_name + " Service";
    std::string description = "Neo blockchain node service";

    SC_HANDLE service = CreateServiceA(sc_manager,                 // SCM database
                                       service_name.c_str(),       // Name of service
                                       display_name.c_str(),       // Service display name
                                       SERVICE_ALL_ACCESS,         // Desired access
                                       SERVICE_WIN32_OWN_PROCESS,  // Service type
                                       SERVICE_AUTO_START,         // Start type
                                       SERVICE_ERROR_NORMAL,       // Error control type
                                       exe_path,                   // Path to service's binary
                                       nullptr,                    // No load ordering group
                                       nullptr,                    // No tag identifier
                                       GetDepends().empty() ? nullptr : GetDepends().c_str(),  // Dependencies
                                       nullptr,                                                // LocalSystem account
                                       nullptr                                                 // No password
    );

    bool success = (service != nullptr);

    if (service)
    {
        CloseServiceHandle(service);
    }
    CloseServiceHandle(sc_manager);

    return success;
}

bool ConsoleServiceBase::UninstallWindowsService()
{
    SC_HANDLE sc_manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!sc_manager)
    {
        return false;
    }

    std::string service_name = GetServiceName();
    SC_HANDLE service = OpenServiceA(sc_manager, service_name.c_str(), DELETE);
    if (!service)
    {
        CloseServiceHandle(sc_manager);
        return false;
    }

    bool success = DeleteService(service);

    CloseServiceHandle(service);
    CloseServiceHandle(sc_manager);

    return success;
}
#endif

}  // namespace neo::console_service
