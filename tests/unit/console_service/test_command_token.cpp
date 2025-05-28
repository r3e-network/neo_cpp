#include <gtest/gtest.h>
#include <neo/console_service/command_token.h>

namespace neo::console_service::tests
{
    class CommandTokenTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // Test setup
        }
    };

    TEST_F(CommandTokenTest, TestParseSimpleCommand)
    {
        std::string command_line = "help version";
        auto tokens = CommandToken::Parse(command_line);
        
        EXPECT_EQ(3, tokens.size());  // "help", " ", "version"
        EXPECT_EQ(CommandTokenType::String, tokens[0]->GetType());
        EXPECT_EQ("help", tokens[0]->GetValue());
        EXPECT_EQ(CommandTokenType::Space, tokens[1]->GetType());
        EXPECT_EQ(" ", tokens[1]->GetValue());
        EXPECT_EQ(CommandTokenType::String, tokens[2]->GetType());
        EXPECT_EQ("version", tokens[2]->GetValue());
    }

    TEST_F(CommandTokenTest, TestParseQuotedCommand)
    {
        std::string command_line = "command \"quoted string\"";
        auto tokens = CommandToken::Parse(command_line);
        
        EXPECT_GE(tokens.size(), 4);  // At least: "command", " ", "\"", "quoted string", "\""
        
        // Find the quoted string token
        bool found_quoted = false;
        for (const auto& token : tokens)
        {
            if (token->GetType() == CommandTokenType::String && token->GetValue() == "quoted string")
            {
                found_quoted = true;
                break;
            }
        }
        EXPECT_TRUE(found_quoted);
    }

    TEST_F(CommandTokenTest, TestParseSingleQuotedCommand)
    {
        std::string command_line = "command 'single quoted'";
        auto tokens = CommandToken::Parse(command_line);
        
        EXPECT_GE(tokens.size(), 4);
        
        // Find the quoted string token
        bool found_quoted = false;
        for (const auto& token : tokens)
        {
            if (token->GetType() == CommandTokenType::String && token->GetValue() == "single quoted")
            {
                found_quoted = true;
                break;
            }
        }
        EXPECT_TRUE(found_quoted);
    }

    TEST_F(CommandTokenTest, TestParseEmptyCommand)
    {
        std::string command_line = "";
        auto tokens = CommandToken::Parse(command_line);
        EXPECT_TRUE(tokens.empty());
    }

    TEST_F(CommandTokenTest, TestParseSpacesOnly)
    {
        std::string command_line = "   ";
        auto tokens = CommandToken::Parse(command_line);
        EXPECT_EQ(1, tokens.size());
        EXPECT_EQ(CommandTokenType::Space, tokens[0]->GetType());
        EXPECT_EQ("   ", tokens[0]->GetValue());
    }

    TEST_F(CommandTokenTest, TestToArguments)
    {
        std::string command_line = "help version clear";
        auto tokens = CommandToken::Parse(command_line);
        auto arguments = CommandToken::ToArguments(tokens);
        
        EXPECT_EQ(3, arguments.size());
        EXPECT_EQ("help", arguments[0]);
        EXPECT_EQ("version", arguments[1]);
        EXPECT_EQ("clear", arguments[2]);
    }

    TEST_F(CommandTokenTest, TestToArgumentsWithQuotes)
    {
        std::string command_line = "command \"quoted arg\" normal";
        auto tokens = CommandToken::Parse(command_line);
        auto arguments = CommandToken::ToArguments(tokens);
        
        EXPECT_GE(arguments.size(), 2);
        EXPECT_EQ("command", arguments[0]);
        
        // Find the quoted argument
        bool found_quoted = false;
        for (const auto& arg : arguments)
        {
            if (arg == "quoted arg")
            {
                found_quoted = true;
                break;
            }
        }
        EXPECT_TRUE(found_quoted);
    }

    TEST_F(CommandTokenTest, TestToString)
    {
        std::string command_line = "help version";
        auto tokens = CommandToken::Parse(command_line);
        std::string result = CommandToken::ToString(tokens);
        EXPECT_EQ(command_line, result);
    }

    TEST_F(CommandTokenTest, TestTrim)
    {
        std::string command_line = "  help version  ";
        auto tokens = CommandToken::Parse(command_line);
        
        EXPECT_GT(tokens.size(), 2);  // Should have spaces at start and end
        
        CommandToken::Trim(tokens);
        
        // After trimming, should not start or end with spaces
        if (!tokens.empty())
        {
            EXPECT_NE(CommandTokenType::Space, tokens.front()->GetType());
            EXPECT_NE(CommandTokenType::Space, tokens.back()->GetType());
        }
    }

    TEST_F(CommandTokenTest, TestReadString)
    {
        std::string command_line = "help version";
        auto tokens = CommandToken::Parse(command_line);
        
        std::string first = CommandToken::ReadString(tokens, false);
        EXPECT_EQ("help", first);
        
        // After reading first string, tokens should be modified
        EXPECT_LT(tokens.size(), 3);  // Should have fewer tokens now
    }

    TEST_F(CommandTokenTest, TestReadStringConsumeAll)
    {
        std::string command_line = "help version clear";
        auto tokens = CommandToken::Parse(command_line);
        
        std::string all = CommandToken::ReadString(tokens, true);
        EXPECT_TRUE(all.find("help") != std::string::npos);
        EXPECT_TRUE(all.find("version") != std::string::npos);
        EXPECT_TRUE(all.find("clear") != std::string::npos);
        
        // After consuming all, tokens should be empty
        EXPECT_TRUE(tokens.empty());
    }

    TEST_F(CommandTokenTest, TestReadStringQuoted)
    {
        std::string command_line = "\"quoted string\" normal";
        auto tokens = CommandToken::Parse(command_line);
        
        std::string quoted = CommandToken::ReadString(tokens, false);
        EXPECT_EQ("quoted string", quoted);
    }

    TEST_F(CommandTokenTest, TestCommandSpaceToken)
    {
        int index = 0;
        std::string command_line = "   test";
        auto space_token = CommandSpaceToken::Parse(command_line, index);
        
        EXPECT_EQ(CommandTokenType::Space, space_token->GetType());
        EXPECT_EQ("   ", space_token->GetValue());
        EXPECT_EQ(3, index);  // Index should be advanced
    }

    TEST_F(CommandTokenTest, TestCommandQuoteToken)
    {
        int index = 0;
        std::string command_line = "\"test";
        auto quote_token = CommandQuoteToken::Parse(command_line, index);
        
        EXPECT_EQ(CommandTokenType::Quote, quote_token->GetType());
        EXPECT_EQ("\"", quote_token->GetValue());
        EXPECT_EQ('"', quote_token->GetQuoteChar());
        EXPECT_EQ(1, index);  // Index should be advanced
    }

    TEST_F(CommandTokenTest, TestCommandStringToken)
    {
        int index = 0;
        std::string command_line = "test string";
        auto string_token = CommandStringToken::Parse(command_line, index);
        
        EXPECT_EQ(CommandTokenType::String, string_token->GetType());
        EXPECT_EQ("test", string_token->GetValue());
        EXPECT_EQ(4, index);  // Index should be advanced to space
    }

    TEST_F(CommandTokenTest, TestComplexCommand)
    {
        std::string command_line = "create wallet \"my wallet.json\" password123";
        auto tokens = CommandToken::Parse(command_line);
        auto arguments = CommandToken::ToArguments(tokens);
        
        EXPECT_GE(arguments.size(), 3);
        EXPECT_EQ("create", arguments[0]);
        EXPECT_EQ("wallet", arguments[1]);
        
        // Should contain the quoted filename
        bool found_filename = false;
        for (const auto& arg : arguments)
        {
            if (arg == "my wallet.json")
            {
                found_filename = true;
                break;
            }
        }
        EXPECT_TRUE(found_filename);
    }

    TEST_F(CommandTokenTest, TestEscapedQuotes)
    {
        std::string command_line = "command \"escaped \\\"quote\\\" test\"";
        auto tokens = CommandToken::Parse(command_line);
        auto arguments = CommandToken::ToArguments(tokens, true);  // Remove escapes
        
        // Should find the string with unescaped quotes
        bool found_escaped = false;
        for (const auto& arg : arguments)
        {
            if (arg.find("escaped \"quote\" test") != std::string::npos)
            {
                found_escaped = true;
                break;
            }
        }
        EXPECT_TRUE(found_escaped);
    }
}
