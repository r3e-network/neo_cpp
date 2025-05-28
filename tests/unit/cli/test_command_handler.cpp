#include <gtest/gtest.h>
#include <neo/cli/command_handler.h>

namespace neo::cli::tests
{
    class CommandHandlerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            handler = std::make_unique<CommandHandler>();
        }

        std::unique_ptr<CommandHandler> handler;
    };

    TEST_F(CommandHandlerTest, TestCommandRegistration)
    {
        // Test registering a simple command
        bool command_executed = false;
        
        handler->RegisterCommand("test", "Test command", [&command_executed](const std::vector<std::string>& args) {
            command_executed = true;
            return true;
        });
        
        // Execute the command
        bool result = handler->ExecuteCommand("test", {});
        
        EXPECT_TRUE(result);
        EXPECT_TRUE(command_executed);
    }

    TEST_F(CommandHandlerTest, TestCommandWithArguments)
    {
        std::vector<std::string> received_args;
        
        handler->RegisterCommand("echo", "Echo arguments", [&received_args](const std::vector<std::string>& args) {
            received_args = args;
            return true;
        });
        
        std::vector<std::string> test_args = {"hello", "world", "123"};
        bool result = handler->ExecuteCommand("echo", test_args);
        
        EXPECT_TRUE(result);
        EXPECT_EQ(test_args, received_args);
    }

    TEST_F(CommandHandlerTest, TestUnknownCommand)
    {
        bool result = handler->ExecuteCommand("unknown_command", {});
        EXPECT_FALSE(result);
    }

    TEST_F(CommandHandlerTest, TestCommandOverwrite)
    {
        int execution_count = 0;
        
        // Register first command
        handler->RegisterCommand("test", "Test command 1", [&execution_count](const std::vector<std::string>& args) {
            execution_count = 1;
            return true;
        });
        
        // Register second command with same name (should overwrite)
        handler->RegisterCommand("test", "Test command 2", [&execution_count](const std::vector<std::string>& args) {
            execution_count = 2;
            return true;
        });
        
        handler->ExecuteCommand("test", {});
        
        EXPECT_EQ(2, execution_count); // Should execute the second command
    }

    TEST_F(CommandHandlerTest, TestCommandFailure)
    {
        handler->RegisterCommand("fail", "Failing command", [](const std::vector<std::string>& args) {
            return false; // Command fails
        });
        
        bool result = handler->ExecuteCommand("fail", {});
        EXPECT_FALSE(result);
    }

    TEST_F(CommandHandlerTest, TestCommandException)
    {
        handler->RegisterCommand("exception", "Exception command", [](const std::vector<std::string>& args) {
            throw std::runtime_error("Test exception");
            return true;
        });
        
        // Should handle exception gracefully
        bool result = handler->ExecuteCommand("exception", {});
        EXPECT_FALSE(result);
    }

    TEST_F(CommandHandlerTest, TestGetCommandList)
    {
        handler->RegisterCommand("cmd1", "Command 1", [](const std::vector<std::string>& args) { return true; });
        handler->RegisterCommand("cmd2", "Command 2", [](const std::vector<std::string>& args) { return true; });
        handler->RegisterCommand("cmd3", "Command 3", [](const std::vector<std::string>& args) { return true; });
        
        auto commands = handler->GetCommandList();
        
        EXPECT_EQ(3, commands.size());
        EXPECT_TRUE(commands.find("cmd1") != commands.end());
        EXPECT_TRUE(commands.find("cmd2") != commands.end());
        EXPECT_TRUE(commands.find("cmd3") != commands.end());
    }

    TEST_F(CommandHandlerTest, TestGetCommandDescription)
    {
        std::string description = "Test command description";
        handler->RegisterCommand("test", description, [](const std::vector<std::string>& args) { return true; });
        
        auto commands = handler->GetCommandList();
        EXPECT_EQ(description, commands["test"]);
    }

    TEST_F(CommandHandlerTest, TestEmptyCommandName)
    {
        // Should handle empty command name gracefully
        bool result = handler->ExecuteCommand("", {});
        EXPECT_FALSE(result);
    }

    TEST_F(CommandHandlerTest, TestCaseSensitivity)
    {
        bool executed = false;
        handler->RegisterCommand("Test", "Test command", [&executed](const std::vector<std::string>& args) {
            executed = true;
            return true;
        });
        
        // Test exact case
        executed = false;
        bool result1 = handler->ExecuteCommand("Test", {});
        EXPECT_TRUE(result1);
        EXPECT_TRUE(executed);
        
        // Test different case (should fail if case-sensitive)
        executed = false;
        bool result2 = handler->ExecuteCommand("test", {});
        EXPECT_FALSE(result2);
        EXPECT_FALSE(executed);
    }

    TEST_F(CommandHandlerTest, TestLargeNumberOfCommands)
    {
        // Register many commands
        for (int i = 0; i < 1000; ++i)
        {
            std::string cmd_name = "cmd" + std::to_string(i);
            handler->RegisterCommand(cmd_name, "Command " + std::to_string(i), 
                [i](const std::vector<std::string>& args) { 
                    return i % 2 == 0; // Even commands succeed, odd fail
                });
        }
        
        auto commands = handler->GetCommandList();
        EXPECT_EQ(1000, commands.size());
        
        // Test some commands
        EXPECT_TRUE(handler->ExecuteCommand("cmd0", {}));   // Even - should succeed
        EXPECT_FALSE(handler->ExecuteCommand("cmd1", {}));  // Odd - should fail
        EXPECT_TRUE(handler->ExecuteCommand("cmd100", {})); // Even - should succeed
        EXPECT_FALSE(handler->ExecuteCommand("cmd101", {})); // Odd - should fail
    }

    TEST_F(CommandHandlerTest, TestCommandWithManyArguments)
    {
        std::vector<std::string> received_args;
        
        handler->RegisterCommand("many_args", "Command with many arguments", 
            [&received_args](const std::vector<std::string>& args) {
                received_args = args;
                return true;
            });
        
        // Create many arguments
        std::vector<std::string> many_args;
        for (int i = 0; i < 100; ++i)
        {
            many_args.push_back("arg" + std::to_string(i));
        }
        
        bool result = handler->ExecuteCommand("many_args", many_args);
        
        EXPECT_TRUE(result);
        EXPECT_EQ(many_args, received_args);
        EXPECT_EQ(100, received_args.size());
    }

    TEST_F(CommandHandlerTest, TestSpecialCharactersInArguments)
    {
        std::vector<std::string> received_args;
        
        handler->RegisterCommand("special", "Command with special characters", 
            [&received_args](const std::vector<std::string>& args) {
                received_args = args;
                return true;
            });
        
        std::vector<std::string> special_args = {
            "hello world",
            "arg with spaces",
            "arg\"with\"quotes",
            "arg'with'apostrophes",
            "arg\\with\\backslashes",
            "arg/with/slashes",
            "arg@with#special$chars%",
            "unicode: 世界",
            ""  // Empty argument
        };
        
        bool result = handler->ExecuteCommand("special", special_args);
        
        EXPECT_TRUE(result);
        EXPECT_EQ(special_args, received_args);
    }

    TEST_F(CommandHandlerTest, TestConcurrentExecution)
    {
        std::atomic<int> execution_count{0};
        
        handler->RegisterCommand("concurrent", "Concurrent command", 
            [&execution_count](const std::vector<std::string>& args) {
                execution_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return true;
            });
        
        // Execute command concurrently
        std::vector<std::future<bool>> futures;
        for (int i = 0; i < 10; ++i)
        {
            futures.push_back(std::async(std::launch::async, [this]() {
                return handler->ExecuteCommand("concurrent", {});
            }));
        }
        
        // Wait for all executions to complete
        for (auto& future : futures)
        {
            EXPECT_TRUE(future.get());
        }
        
        EXPECT_EQ(10, execution_count.load());
    }
}
