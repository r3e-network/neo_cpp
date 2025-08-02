#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace neo::cli
{

// Forward declaration
class CLIService;

/**
 * @brief Base class for all CLI commands
 */
class Command
{
  public:
    virtual ~Command() = default;

    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual std::string GetUsage() const = 0;
    virtual bool Execute(CLIService* service, const std::vector<std::string>& args) = 0;
};

/**
 * @brief Registry for all available CLI commands
 */
class CommandRegistry
{
  public:
    explicit CommandRegistry(CLIService* service);
    ~CommandRegistry();

    // Command registration
    void RegisterCommand(std::unique_ptr<Command> command);
    void RegisterBuiltinCommands();

    // Command execution
    bool ExecuteCommand(const std::string& name, const std::vector<std::string>& args);

    // Help
    void DisplayHelp();
    void DisplayCommandHelp(const std::string& command_name);

    // Command listing
    std::vector<std::string> GetCommandNames() const;
    Command* GetCommand(const std::string& name);

  private:
    CLIService* service_;
    std::map<std::string, std::unique_ptr<Command>> commands_;
};

// Built-in command classes
class HelpCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "help";
    }
    std::string GetDescription() const override
    {
        return "Display help information";
    }
    std::string GetUsage() const override
    {
        return "help [command]";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class StatusCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "status";
    }
    std::string GetDescription() const override
    {
        return "Display node status";
    }
    std::string GetUsage() const override
    {
        return "status";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class ExitCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "exit";
    }
    std::string GetDescription() const override
    {
        return "Exit the application";
    }
    std::string GetUsage() const override
    {
        return "exit";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class ShowCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "show";
    }
    std::string GetDescription() const override
    {
        return "Show various information";
    }
    std::string GetUsage() const override
    {
        return "show <state|pool|account|asset|contract>";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class WalletCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "wallet";
    }
    std::string GetDescription() const override
    {
        return "Wallet operations";
    }
    std::string GetUsage() const override
    {
        return "wallet <open|close|create|list|import> [args]";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class SendCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "send";
    }
    std::string GetDescription() const override
    {
        return "Send assets";
    }
    std::string GetUsage() const override
    {
        return "send <asset> <to> <amount>";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class InvokeCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "invoke";
    }
    std::string GetDescription() const override
    {
        return "Invoke smart contract";
    }
    std::string GetUsage() const override
    {
        return "invoke <scripthash> <method> [params]";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class DeployCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "deploy";
    }
    std::string GetDescription() const override
    {
        return "Deploy smart contract";
    }
    std::string GetUsage() const override
    {
        return "deploy <neffile> <manifest>";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class VoteCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "vote";
    }
    std::string GetDescription() const override
    {
        return "Vote for consensus nodes";
    }
    std::string GetUsage() const override
    {
        return "vote <pubkey>";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class ClaimCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "claim";
    }
    std::string GetDescription() const override
    {
        return "Claim GAS";
    }
    std::string GetUsage() const override
    {
        return "claim";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class PluginsCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "plugins";
    }
    std::string GetDescription() const override
    {
        return "Manage plugins";
    }
    std::string GetUsage() const override
    {
        return "plugins [list|install|uninstall]";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class ExportCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "export";
    }
    std::string GetDescription() const override
    {
        return "Export blockchain data";
    }
    std::string GetUsage() const override
    {
        return "export blocks <start> <count> [path]";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

class ImportCommand : public Command
{
  public:
    std::string GetName() const override
    {
        return "import";
    }
    std::string GetDescription() const override
    {
        return "Import blockchain data";
    }
    std::string GetUsage() const override
    {
        return "import blocks <path>";
    }
    bool Execute(CLIService* service, const std::vector<std::string>& args) override;
};

}  // namespace neo::cli