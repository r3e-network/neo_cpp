#include <gtest/gtest.h>
#include <iostream>
#include <neo/console_service/console_service_base.h>
#include <sstream>

namespace neo::console_service::tests
{
class TestConsoleService : public ConsoleServiceBase
{
  public:
    std::string GetServiceName() const override
    {
        return "TestService";
    }

    std::string GetPrompt() const override
    {
        return "test";
    }

    bool test_command_called = false;
    std::string last_command;

    void TestCommand(const std::string& arg)
    {
        test_command_called = true;
        last_command = arg;
    }

    // Expose protected methods for testing
    using ConsoleServiceBase::OnClear;
    using ConsoleServiceBase::OnCommand;
    using ConsoleServiceBase::OnExit;
    using ConsoleServiceBase::OnHelpCommand;
    using ConsoleServiceBase::OnVersion;
};

class ConsoleServiceBaseTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        service = std::make_unique<TestConsoleService>();

        // Save original cout buffer
        original_cout_ = std::cout.rdbuf();
        // Redirect cout to our stringstream
        std::cout.rdbuf(output_stream_.rdbuf());
    }

    void TearDown() override
    {
        // Restore original cout buffer
        std::cout.rdbuf(original_cout_);
    }

    std::string GetOutput()
    {
        std::string result = output_stream_.str();
        output_stream_.str("");  // Clear the stream
        output_stream_.clear();
        return result;
    }

    std::unique_ptr<TestConsoleService> service;

  private:
    std::ostringstream output_stream_;
    std::streambuf* original_cout_;
};

TEST_F(ConsoleServiceBaseTest, TestServiceName)
{
    EXPECT_EQ("TestService", service->GetServiceName());
}

TEST_F(ConsoleServiceBaseTest, TestPrompt)
{
    EXPECT_EQ("test", service->GetPrompt());
}

TEST_F(ConsoleServiceBaseTest, TestShowPrompt)
{
    EXPECT_TRUE(service->GetShowPrompt());  // Default should be true

    service->SetShowPrompt(false);
    EXPECT_FALSE(service->GetShowPrompt());

    service->SetShowPrompt(true);
    EXPECT_TRUE(service->GetShowPrompt());
}

TEST_F(ConsoleServiceBaseTest, TestHelpCommand)
{
    service->OnHelpCommand();
    std::string output = GetOutput();

    EXPECT_TRUE(output.find("Base Commands:") != std::string::npos);
    EXPECT_TRUE(output.find("help") != std::string::npos);
    EXPECT_TRUE(output.find("clear") != std::string::npos);
    EXPECT_TRUE(output.find("version") != std::string::npos);
    EXPECT_TRUE(output.find("exit") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestHelpSpecificCommand)
{
    service->OnHelpCommand("help");
    std::string output = GetOutput();

    EXPECT_TRUE(output.find("Shows help information") != std::string::npos);
    EXPECT_TRUE(output.find("You can call this command like this:") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestHelpUnknownCommand)
{
    service->OnHelpCommand("unknown");
    std::string output = GetOutput();

    EXPECT_TRUE(output.find("Command not found") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestClearCommand)
{
    // Test that clear command doesn't crash
    EXPECT_NO_THROW(service->OnClear());
}

TEST_F(ConsoleServiceBaseTest, TestVersionCommand)
{
    service->OnVersion();
    std::string output = GetOutput();

    EXPECT_TRUE(output.find("Neo C++ Node") != std::string::npos);
    EXPECT_TRUE(output.find("v1.0.0") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestOnStart)
{
    std::vector<std::string> args = {"arg1", "arg2"};
    EXPECT_TRUE(service->OnStart(args));
}

TEST_F(ConsoleServiceBaseTest, TestOnStop)
{
    // Test that OnStop doesn't crash
    EXPECT_NO_THROW(service->OnStop());
}

TEST_F(ConsoleServiceBaseTest, TestCommandHandlerRegistration)
{
    // Test registering a string handler
    EXPECT_NO_THROW({
        service->RegisterCommandHandler<std::string>(
            [](std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all) -> std::string { return "test"; });
    });

    // Test registering an int handler
    EXPECT_NO_THROW({
        service->RegisterCommandHandler<int>(
            [](std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all) -> int { return 42; });
    });
}

TEST_F(ConsoleServiceBaseTest, TestCommandRegistration)
{
    auto instance = std::make_shared<TestConsoleService>();
    EXPECT_NO_THROW(service->RegisterCommand(instance, "test"));
}

TEST_F(ConsoleServiceBaseTest, TestBuiltInCommands)
{
    // Test help command
    EXPECT_TRUE(service->OnCommand("help"));
    std::string help_output = GetOutput();
    EXPECT_TRUE(help_output.find("Base Commands:") != std::string::npos);

    // Test clear command
    EXPECT_TRUE(service->OnCommand("clear"));

    // Test version command
    EXPECT_TRUE(service->OnCommand("version"));
    std::string version_output = GetOutput();
    EXPECT_TRUE(version_output.find("Neo C++ Node") != std::string::npos);

    // Test exit command (note: this sets running_ to false)
    EXPECT_TRUE(service->OnCommand("exit"));
}

TEST_F(ConsoleServiceBaseTest, TestEmptyCommand)
{
    EXPECT_TRUE(service->OnCommand(""));  // Empty command should return true
}

TEST_F(ConsoleServiceBaseTest, TestUnknownCommand)
{
    EXPECT_FALSE(service->OnCommand("unknown_command"));
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Command not found") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestCommandWithArguments)
{
    EXPECT_TRUE(service->OnCommand("help version"));
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Show the current version") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestCaseInsensitiveCommands)
{
    // Test that commands are case insensitive
    EXPECT_TRUE(service->OnCommand("HELP"));
    std::string output1 = GetOutput();
    EXPECT_TRUE(output1.find("Base Commands:") != std::string::npos);

    EXPECT_TRUE(service->OnCommand("Help"));
    std::string output2 = GetOutput();
    EXPECT_TRUE(output2.find("Base Commands:") != std::string::npos);

    EXPECT_TRUE(service->OnCommand("hElP"));
    std::string output3 = GetOutput();
    EXPECT_TRUE(output3.find("Base Commands:") != std::string::npos);
}

TEST_F(ConsoleServiceBaseTest, TestGetDepends)
{
    EXPECT_EQ("", service->GetDepends());  // Default implementation returns empty string
}

TEST_F(ConsoleServiceBaseTest, TestRunWithSpecialArgs)
{
    // Test /install argument
    std::vector<std::string> install_args = {"/install"};
    EXPECT_NO_THROW(service->Run(install_args));
    std::string install_output = GetOutput();
    // Should show some message about installation

    // Test /uninstall argument
    std::vector<std::string> uninstall_args = {"/uninstall"};
    EXPECT_NO_THROW(service->Run(uninstall_args));
    std::string uninstall_output = GetOutput();
    // Should show some message about uninstallation
}

TEST_F(ConsoleServiceBaseTest, TestCommandParsing)
{
    // Test command parsing with quotes
    EXPECT_TRUE(service->OnCommand("help \"version command\""));

    // Test command parsing with multiple arguments
    EXPECT_TRUE(service->OnCommand("help version clear"));
}

TEST_F(ConsoleServiceBaseTest, TestExceptionHandling)
{
    // Commands that throw exceptions should be handled gracefully
    // This is tested implicitly by other tests not crashing
    EXPECT_NO_THROW(service->OnCommand("help unknown_command"));
}
}  // namespace neo::console_service::tests
