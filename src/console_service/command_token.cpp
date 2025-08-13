/**
 * @file command_token.cpp
 * @brief Command Token
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/console_service/command_token.h>

#include <sstream>
#include <stdexcept>

namespace neo::console_service
{
// CommandToken implementation
CommandToken::CommandToken(CommandTokenType type, int offset) : type_(type), offset_(offset) {}

CommandTokenType CommandToken::GetType() const { return type_; }

int CommandToken::GetOffset() const { return offset_; }

const std::string& CommandToken::GetValue() const { return value_; }

void CommandToken::SetValue(const std::string& value) { value_ = value; }

std::vector<std::shared_ptr<CommandToken>> CommandToken::Parse(const std::string& command_line)
{
    std::vector<std::shared_ptr<CommandToken>> tokens;
    std::shared_ptr<CommandToken> last_token = nullptr;

    for (int index = 0, count = static_cast<int>(command_line.length()); index < count;)
    {
        switch (command_line[index])
        {
            case ' ':
            {
                last_token = CommandSpaceToken::Parse(command_line, index);
                tokens.push_back(last_token);
                break;
            }
            case '"':
            case '\'':
            {
                // Check if we're in a different quote context
                auto quote = std::dynamic_pointer_cast<CommandQuoteToken>(last_token);
                if (quote && quote->GetQuoteChar() != command_line[index])
                {
                    goto default_case;
                }

                last_token = CommandQuoteToken::Parse(command_line, index);
                tokens.push_back(last_token);
                break;
            }
            default:
            default_case:
            {
                auto quote = std::dynamic_pointer_cast<CommandQuoteToken>(last_token);
                last_token = CommandStringToken::Parse(command_line, index, quote);

                if (last_token)
                {
                    tokens.push_back(last_token);
                }
                break;
            }
        }
    }

    return tokens;
}

std::vector<std::string> CommandToken::ToArguments(const std::vector<std::shared_ptr<CommandToken>>& tokens,
                                                   bool remove_escape)
{
    std::vector<std::string> arguments;
    std::shared_ptr<CommandToken> last_token = nullptr;

    for (const auto& token : tokens)
    {
        auto string_token = std::dynamic_pointer_cast<CommandStringToken>(token);
        if (string_token)
        {
            auto quote = std::dynamic_pointer_cast<CommandQuoteToken>(last_token);
            if (remove_escape && quote)
            {
                // Remove escape characters
                std::string value = string_token->GetValue();
                std::string quote_str(1, quote->GetQuoteChar());
                std::string escaped_quote = "\\" + quote_str;

                size_t pos = 0;
                while ((pos = value.find(escaped_quote, pos)) != std::string::npos)
                {
                    value.replace(pos, escaped_quote.length(), quote_str);
                    pos += quote_str.length();
                }

                arguments.push_back(value);
            }
            else
            {
                arguments.push_back(string_token->GetValue());
            }
        }

        last_token = token;
    }

    return arguments;
}

std::string CommandToken::ToString(const std::vector<std::shared_ptr<CommandToken>>& tokens)
{
    std::ostringstream oss;
    for (const auto& token : tokens)
    {
        oss << token->GetValue();
    }
    return oss.str();
}

void CommandToken::Trim(std::vector<std::shared_ptr<CommandToken>>& args)
{
    // Trim start
    while (!args.empty() && args.front()->GetType() == CommandTokenType::Space)
    {
        args.erase(args.begin());
    }

    // Trim end
    while (!args.empty() && args.back()->GetType() == CommandTokenType::Space)
    {
        args.pop_back();
    }
}

std::string CommandToken::ReadString(std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all)
{
    Trim(args);

    bool quoted = false;

    if (!args.empty() && args.front()->GetType() == CommandTokenType::Quote)
    {
        quoted = true;
        args.erase(args.begin());
    }
    else
    {
        if (consume_all)
        {
            // Return all if it's not quoted
            std::string result = ToString(args);
            args.clear();
            return result;
        }
    }

    if (!args.empty())
    {
        auto& front = args.front();

        if (front->GetType() == CommandTokenType::Quote)
        {
            if (quoted)
            {
                args.erase(args.begin());
                return "";
            }
            throw std::invalid_argument("Unmatched quote");
        }
        else if (front->GetType() == CommandTokenType::Space)
        {
            throw std::invalid_argument("Unmatched space");
        }
        else if (front->GetType() == CommandTokenType::String)
        {
            std::string result = front->GetValue();
            args.erase(args.begin());

            if (quoted && !args.empty() && args.front()->GetType() == CommandTokenType::Quote)
            {
                // Remove last quote
                args.erase(args.begin());
            }

            return result;
        }
    }

    return "";
}

// CommandSpaceToken implementation
CommandSpaceToken::CommandSpaceToken(int offset) : CommandToken(CommandTokenType::Space, offset) {}

std::shared_ptr<CommandSpaceToken> CommandSpaceToken::Parse(const std::string& command_line, int& index)
{
    int start = index;
    std::ostringstream oss;

    while (index < static_cast<int>(command_line.length()) && command_line[index] == ' ')
    {
        oss << command_line[index];
        index++;
    }

    auto token = std::make_shared<CommandSpaceToken>(start);
    token->SetValue(oss.str());
    return token;
}

void CommandSpaceToken::Trim(std::vector<std::shared_ptr<CommandToken>>& args) { CommandToken::Trim(args); }

// CommandQuoteToken implementation
CommandQuoteToken::CommandQuoteToken(int offset, char quote_char)
    : CommandToken(CommandTokenType::Quote, offset), quote_char_(quote_char)
{
    value_ = std::string(1, quote_char);
}

std::shared_ptr<CommandQuoteToken> CommandQuoteToken::Parse(const std::string& command_line, int& index)
{
    char quote_char = command_line[index];
    auto token = std::make_shared<CommandQuoteToken>(index, quote_char);
    index++;
    return token;
}

char CommandQuoteToken::GetQuoteChar() const { return quote_char_; }

// CommandStringToken implementation
CommandStringToken::CommandStringToken(int offset, const std::string& value)
    : CommandToken(CommandTokenType::String, offset)
{
    value_ = value;
}

std::shared_ptr<CommandStringToken> CommandStringToken::Parse(const std::string& command_line, int& index,
                                                              std::shared_ptr<CommandQuoteToken> quote)
{
    int start = index;
    std::ostringstream oss;

    while (index < static_cast<int>(command_line.length()))
    {
        char ch = command_line[index];

        if (quote)
        {
            // Inside quotes
            if (ch == quote->GetQuoteChar())
            {
                break;  // End of quoted string
            }
            oss << ch;
            index++;
        }
        else
        {
            // Outside quotes
            if (ch == ' ' || ch == '"' || ch == '\'')
            {
                break;  // End of unquoted string
            }
            oss << ch;
            index++;
        }
    }

    std::string value = oss.str();
    if (value.empty())
    {
        return nullptr;
    }

    return std::make_shared<CommandStringToken>(start, value);
}
}  // namespace neo::console_service
