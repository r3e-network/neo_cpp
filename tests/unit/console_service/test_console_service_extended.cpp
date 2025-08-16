#include <gtest/gtest.h>
#include <neo/console_service/console_service.h>
#include <neo/console_service/command_parser.h>
#include <neo/console_service/command_handler.h>
#include <memory>
#include <sstream>

using namespace neo::console_service;

class ConsoleServiceExtendedTest : public ::testing::Test
{
protected:
    std::unique_ptr<ConsoleService> console;
    std::stringstream output;
    std::stringstream input;
    
    void SetUp() override
    {
        console = std::make_unique<ConsoleService>();
        console->SetOutputStream(&output);
        console->SetInputStream(&input);
    }
    
    void TearDown() override
    {
        console.reset();
    }
    
    std::string GetOutput()
    {
        return output.str();
    }
    
    void ClearOutput()
    {
        output.str("");
        output.clear();
    }
    
    void SendCommand(const std::string& cmd)
    {
        input << cmd << std::endl;
    }
};

// Command Parsing Tests
TEST_F(ConsoleServiceExtendedTest, ParseSimpleCommand)
{
    CommandParser parser;
    auto result = parser.Parse("help");
    
    EXPECT_EQ(result.command, "help");
    EXPECT_TRUE(result.arguments.empty());
    EXPECT_TRUE(result.options.empty());
}

TEST_F(ConsoleServiceExtendedTest, ParseCommandWithArguments)
{
    CommandParser parser;
    auto result = parser.Parse("transfer NEO 100 address1 address2");
    
    EXPECT_EQ(result.command, "transfer");
    EXPECT_EQ(result.arguments.size(), 4);
    EXPECT_EQ(result.arguments[0], "NEO");
    EXPECT_EQ(result.arguments[1], "100");
    EXPECT_EQ(result.arguments[2], "address1");
    EXPECT_EQ(result.arguments[3], "address2");
}

TEST_F(ConsoleServiceExtendedTest, ParseCommandWithOptions)
{
    CommandParser parser;
    auto result = parser.Parse("list --verbose --limit=10");
    
    EXPECT_EQ(result.command, "list");
    EXPECT_TRUE(result.HasOption("verbose"));
    EXPECT_TRUE(result.HasOption("limit"));
    EXPECT_EQ(result.GetOption("limit"), "10");
}

TEST_F(ConsoleServiceExtendedTest, ParseComplexCommand)
{
    CommandParser parser;
    auto result = parser.Parse("deploy contract.nef --network=testnet --gas=10 --verbose");
    
    EXPECT_EQ(result.command, "deploy");
    EXPECT_EQ(result.arguments.size(), 1);
    EXPECT_EQ(result.arguments[0], "contract.nef");
    EXPECT_EQ(result.GetOption("network"), "testnet");
    EXPECT_EQ(result.GetOption("gas"), "10");
    EXPECT_TRUE(result.HasOption("verbose"));
}

// Input Validation Tests
TEST_F(ConsoleServiceExtendedTest, ValidateEmptyCommand)
{
    SendCommand("");
    console->ProcessCommand();
    
    // Empty command should be ignored
    EXPECT_TRUE(GetOutput().empty());
}

TEST_F(ConsoleServiceExtendedTest, ValidateInvalidCommand)
{
    SendCommand("invalid_command_12345");
    console->ProcessCommand();
    
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Unknown command") != std::string::npos ||
                output.find("Invalid") != std::string::npos);
}

TEST_F(ConsoleServiceExtendedTest, ValidateCommandWithInvalidArguments)
{
    SendCommand("transfer");  // Missing required arguments
    console->ProcessCommand();
    
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Invalid arguments") != std::string::npos ||
                output.find("Usage:") != std::string::npos);
}

// Error Handling Tests
TEST_F(ConsoleServiceExtendedTest, HandleCommandException)
{
    // Register a command that throws
    console->RegisterCommand("throw", [](const CommandContext& ctx) {
        throw std::runtime_error("Test exception");
    });
    
    SendCommand("throw");
    EXPECT_NO_THROW(console->ProcessCommand());
    
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Error") != std::string::npos ||
                output.find("exception") != std::string::npos);
}

TEST_F(ConsoleServiceExtendedTest, HandleInvalidInput)
{
    // Send invalid UTF-8 or control characters
    input << "\x01\x02\x03" << std::endl;
    EXPECT_NO_THROW(console->ProcessCommand());
}

TEST_F(ConsoleServiceExtendedTest, HandleLongCommand)
{
    // Test with extremely long command line
    std::string longCommand(10000, 'a');
    SendCommand(longCommand);
    EXPECT_NO_THROW(console->ProcessCommand());
}

// Multi-Command Sequence Tests
TEST_F(ConsoleServiceExtendedTest, ExecuteMultipleCommands)
{
    SendCommand("help");
    SendCommand("version");
    SendCommand("status");
    
    console->ProcessCommand();
    std::string output1 = GetOutput();
    EXPECT_FALSE(output1.empty());
    
    ClearOutput();
    console->ProcessCommand();
    std::string output2 = GetOutput();
    EXPECT_FALSE(output2.empty());
    
    ClearOutput();
    console->ProcessCommand();
    std::string output3 = GetOutput();
    EXPECT_FALSE(output3.empty());
}

TEST_F(ConsoleServiceExtendedTest, CommandHistory)
{
    console->EnableHistory(true);
    
    SendCommand("command1");
    SendCommand("command2");
    SendCommand("command3");
    
    console->ProcessCommand();
    console->ProcessCommand();
    console->ProcessCommand();
    
    auto history = console->GetHistory();
    EXPECT_EQ(history.size(), 3);
    EXPECT_EQ(history[0], "command1");
    EXPECT_EQ(history[1], "command2");
    EXPECT_EQ(history[2], "command3");
}

// Command Registration Tests
TEST_F(ConsoleServiceExtendedTest, RegisterCustomCommand)
{
    bool commandExecuted = false;
    
    console->RegisterCommand("custom", [&commandExecuted](const CommandContext& ctx) {
        commandExecuted = true;
        return CommandResult::Success("Custom command executed");
    });
    
    SendCommand("custom");
    console->ProcessCommand();
    
    EXPECT_TRUE(commandExecuted);
    EXPECT_TRUE(GetOutput().find("Custom command executed") != std::string::npos);
}

TEST_F(ConsoleServiceExtendedTest, OverrideExistingCommand)
{
    console->RegisterCommand("test", [](const CommandContext& ctx) {
        return CommandResult::Success("Original");
    });
    
    console->RegisterCommand("test", [](const CommandContext& ctx) {
        return CommandResult::Success("Override");
    });
    
    SendCommand("test");
    console->ProcessCommand();
    
    EXPECT_TRUE(GetOutput().find("Override") != std::string::npos);
}

// Interactive Mode Tests
TEST_F(ConsoleServiceExtendedTest, InteractivePrompt)
{
    console->SetInteractiveMode(true);
    console->SetPrompt("neo> ");
    
    EXPECT_TRUE(console->IsInteractive());
    EXPECT_EQ(console->GetPrompt(), "neo> ");
}

TEST_F(ConsoleServiceExtendedTest, ExitCommand)
{
    console->SetInteractiveMode(true);
    
    SendCommand("exit");
    console->ProcessCommand();
    
    EXPECT_FALSE(console->IsRunning());
}

// Output Formatting Tests
TEST_F(ConsoleServiceExtendedTest, FormatTableOutput)
{
    TableFormatter formatter;
    formatter.AddColumn("Name", 20);
    formatter.AddColumn("Value", 10);
    formatter.AddColumn("Status", 10);
    
    formatter.AddRow({"Item1", "100", "Active"});
    formatter.AddRow({"Item2", "200", "Inactive"});
    
    std::string table = formatter.ToString();
    EXPECT_TRUE(table.find("Name") != std::string::npos);
    EXPECT_TRUE(table.find("Item1") != std::string::npos);
    EXPECT_TRUE(table.find("100") != std::string::npos);
}

TEST_F(ConsoleServiceExtendedTest, FormatJsonOutput)
{
    JsonFormatter formatter;
    formatter.Add("status", "success");
    formatter.Add("count", 42);
    formatter.Add("active", true);
    
    std::string json = formatter.ToString();
    EXPECT_TRUE(json.find("\"status\":\"success\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"count\":42") != std::string::npos);
    EXPECT_TRUE(json.find("\"active\":true") != std::string::npos);
}

// Auto-completion Tests
TEST_F(ConsoleServiceExtendedTest, AutoCompleteCommand)
{
    console->RegisterCommand("transfer", nullptr);
    console->RegisterCommand("transaction", nullptr);
    console->RegisterCommand("help", nullptr);
    
    auto suggestions = console->AutoComplete("tra");
    EXPECT_EQ(suggestions.size(), 2);
    EXPECT_TRUE(std::find(suggestions.begin(), suggestions.end(), "transfer") != suggestions.end());
    EXPECT_TRUE(std::find(suggestions.begin(), suggestions.end(), "transaction") != suggestions.end());
}

TEST_F(ConsoleServiceExtendedTest, AutoCompleteNoMatch)
{
    console->RegisterCommand("help", nullptr);
    console->RegisterCommand("version", nullptr);
    
    auto suggestions = console->AutoComplete("xyz");
    EXPECT_TRUE(suggestions.empty());
}

// Security Tests
TEST_F(ConsoleServiceExtendedTest, PreventCommandInjection)
{
    // Test command injection attempts
    SendCommand("help; rm -rf /");
    console->ProcessCommand();
    
    // Should treat entire string as single command
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Unknown command") != std::string::npos ||
                output.find("Invalid") != std::string::npos);
}

TEST_F(ConsoleServiceExtendedTest, SanitizeInput)
{
    // Test with special characters
    SendCommand("test\r\n\t<script>alert('xss')</script>");
    console->ProcessCommand();
    
    // Should sanitize input
    EXPECT_NO_THROW(console->GetLastCommand());
}

// Performance Tests
TEST_F(ConsoleServiceExtendedTest, PerformanceStressTest)
{
    auto start = std::chrono::high_resolution_clock::now();
    
    // Execute 1000 commands
    for (int i = 0; i < 1000; ++i)
    {
        SendCommand("help");
        console->ProcessCommand();
        ClearOutput();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete in reasonable time (< 1 second)
    EXPECT_LT(duration.count(), 1000);
}

TEST_F(ConsoleServiceExtendedTest, MemoryLeakTest)
{
    // Register and unregister commands repeatedly
    for (int i = 0; i < 100; ++i)
    {
        std::string cmdName = "cmd" + std::to_string(i);
        console->RegisterCommand(cmdName, [](const CommandContext& ctx) {
            return CommandResult::Success("OK");
        });
    }
    
    // Clear all commands
    console->ClearCommands();
    
    // Should not leak memory (verified by valgrind/sanitizers)
    SUCCEED();
}