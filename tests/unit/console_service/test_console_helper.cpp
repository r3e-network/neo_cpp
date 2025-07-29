#include <gtest/gtest.h>
#include <iostream>
#include <neo/console_service/console_helper.h>
#include <sstream>

namespace neo::console_service::tests
{
class ConsoleHelperTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
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

  private:
    std::ostringstream output_stream_;
    std::streambuf* original_cout_;
};

TEST_F(ConsoleHelperTest, TestInfo)
{
    ConsoleHelper::Info("Test", "Message");
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Test") != std::string::npos);
    EXPECT_TRUE(output.find("Message") != std::string::npos);
}

TEST_F(ConsoleHelperTest, TestInfoMultipleValues)
{
    std::vector<std::string> values = {"Tag1", "Message1", "Tag2", "Message2"};
    ConsoleHelper::Info(values);
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Tag1") != std::string::npos);
    EXPECT_TRUE(output.find("Message1") != std::string::npos);
    EXPECT_TRUE(output.find("Tag2") != std::string::npos);
    EXPECT_TRUE(output.find("Message2") != std::string::npos);
}

TEST_F(ConsoleHelperTest, TestWarning)
{
    ConsoleHelper::Warning("Test warning message");
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Warning:") != std::string::npos);
    EXPECT_TRUE(output.find("Test warning message") != std::string::npos);
}

TEST_F(ConsoleHelperTest, TestError)
{
    ConsoleHelper::Error("Test error message");
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Error:") != std::string::npos);
    EXPECT_TRUE(output.find("Test error message") != std::string::npos);
}

TEST_F(ConsoleHelperTest, TestIsReadingPassword)
{
    // Initially should not be reading password
    EXPECT_FALSE(ConsoleHelper::IsReadingPassword());

    // Note: Testing actual password reading would require mocking stdin,
    // which is complex for this test. We'll test the flag state instead.
}

TEST_F(ConsoleHelperTest, TestColorOperations)
{
    // Test that color operations don't crash
    EXPECT_NO_THROW(ConsoleHelper::SetForegroundColor(ConsoleColor::Red));
    EXPECT_NO_THROW(ConsoleHelper::SetBackgroundColor(ConsoleColor::Blue));
    EXPECT_NO_THROW(ConsoleHelper::ResetColor());
}

TEST_F(ConsoleHelperTest, TestClear)
{
    // Test that clear operation doesn't crash
    EXPECT_NO_THROW(ConsoleHelper::Clear());
}

TEST_F(ConsoleHelperTest, TestConsoleColorSet)
{
    // Test ConsoleColorSet construction and application
    EXPECT_NO_THROW({
        ConsoleColorSet default_colors;
        default_colors.Apply();
    });

    EXPECT_NO_THROW({
        ConsoleColorSet foreground_only(ConsoleColor::Yellow);
        foreground_only.Apply();
    });

    EXPECT_NO_THROW({
        ConsoleColorSet both_colors(ConsoleColor::Green, ConsoleColor::Black);
        both_colors.Apply();
    });
}

TEST_F(ConsoleHelperTest, TestColorEnumValues)
{
    // Test that all color enum values are valid
    std::vector<ConsoleColor> colors = {
        ConsoleColor::Black,    ConsoleColor::DarkBlue,    ConsoleColor::DarkGreen,  ConsoleColor::DarkCyan,
        ConsoleColor::DarkRed,  ConsoleColor::DarkMagenta, ConsoleColor::DarkYellow, ConsoleColor::Gray,
        ConsoleColor::DarkGray, ConsoleColor::Blue,        ConsoleColor::Green,      ConsoleColor::Cyan,
        ConsoleColor::Red,      ConsoleColor::Magenta,     ConsoleColor::Yellow,     ConsoleColor::White};

    for (auto color : colors)
    {
        EXPECT_NO_THROW(ConsoleHelper::SetForegroundColor(color));
        EXPECT_NO_THROW(ConsoleHelper::SetBackgroundColor(color));
    }

    // Reset to default
    ConsoleHelper::ResetColor();
}

TEST_F(ConsoleHelperTest, TestLogFormatting)
{
    // Test that log messages are properly formatted
    ConsoleHelper::Warning("Warning message");
    std::string warning_output = GetOutput();
    EXPECT_TRUE(warning_output.find("Warning: Warning message") != std::string::npos);

    ConsoleHelper::Error("Error message");
    std::string error_output = GetOutput();
    EXPECT_TRUE(error_output.find("Error: Error message") != std::string::npos);
}

TEST_F(ConsoleHelperTest, TestEmptyMessages)
{
    // Test handling of empty messages
    EXPECT_NO_THROW(ConsoleHelper::Info("", ""));
    EXPECT_NO_THROW(ConsoleHelper::Warning(""));
    EXPECT_NO_THROW(ConsoleHelper::Error(""));

    std::vector<std::string> empty_values;
    EXPECT_NO_THROW(ConsoleHelper::Info(empty_values));
}

TEST_F(ConsoleHelperTest, TestSpecialCharacters)
{
    // Test handling of special characters
    ConsoleHelper::Info("Special", "Characters: !@#$%^&*()");
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Special") != std::string::npos);
    EXPECT_TRUE(output.find("Characters: !@#$%^&*()") != std::string::npos);
}

TEST_F(ConsoleHelperTest, TestUnicodeCharacters)
{
    // Test handling of Unicode characters
    ConsoleHelper::Info("Unicode", "Test: 世界 🌍");
    std::string output = GetOutput();
    EXPECT_TRUE(output.find("Unicode") != std::string::npos);
    // Note: Unicode output testing depends on console encoding
}
}  // namespace neo::console_service::tests
