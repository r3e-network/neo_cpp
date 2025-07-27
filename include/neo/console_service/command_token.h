#pragma once

#include <memory>
#include <string>
#include <vector>

namespace neo::console_service
{
/**
 * @brief Command token types.
 */
enum class CommandTokenType
{
    Space,
    Quote,
    String
};

/**
 * @brief Base class for command tokens.
 */
class CommandToken
{
  public:
    /**
     * @brief Constructor.
     * @param type The token type.
     * @param offset The offset in the command line.
     */
    CommandToken(CommandTokenType type, int offset);

    /**
     * @brief Virtual destructor.
     */
    virtual ~CommandToken() = default;

    /**
     * @brief Gets the token type.
     * @return The token type.
     */
    CommandTokenType GetType() const;

    /**
     * @brief Gets the offset.
     * @return The offset.
     */
    int GetOffset() const;

    /**
     * @brief Gets the value.
     * @return The value.
     */
    const std::string& GetValue() const;

    /**
     * @brief Sets the value.
     * @param value The value to set.
     */
    void SetValue(const std::string& value);

    /**
     * @brief Parses a command line into tokens.
     * @param command_line The command line to parse.
     * @return The parsed tokens.
     */
    static std::vector<std::shared_ptr<CommandToken>> Parse(const std::string& command_line);

    /**
     * @brief Creates string arguments from tokens.
     * @param tokens The tokens.
     * @param remove_escape Whether to remove escape characters.
     * @return The string arguments.
     */
    static std::vector<std::string> ToArguments(const std::vector<std::shared_ptr<CommandToken>>& tokens,
                                                bool remove_escape = true);

    /**
     * @brief Creates a string from token list.
     * @param tokens The tokens.
     * @return The concatenated string.
     */
    static std::string ToString(const std::vector<std::shared_ptr<CommandToken>>& tokens);

    /**
     * @brief Trims space tokens from the beginning and end.
     * @param args The token list to trim.
     */
    static void Trim(std::vector<std::shared_ptr<CommandToken>>& args);

    /**
     * @brief Reads a string from the token list.
     * @param args The token list.
     * @param consume_all Whether to consume all tokens if not quoted.
     * @return The read string, or empty string if none.
     */
    static std::string ReadString(std::vector<std::shared_ptr<CommandToken>>& args, bool consume_all);

  protected:
    CommandTokenType type_;
    int offset_;
    std::string value_;
};

/**
 * @brief Space token class.
 */
class CommandSpaceToken : public CommandToken
{
  public:
    /**
     * @brief Constructor.
     * @param offset The offset.
     */
    explicit CommandSpaceToken(int offset);

    /**
     * @brief Parses a space token.
     * @param command_line The command line.
     * @param index The current index (will be updated).
     * @return The parsed space token.
     */
    static std::shared_ptr<CommandSpaceToken> Parse(const std::string& command_line, int& index);

    /**
     * @brief Trims space tokens from the token list.
     * @param args The token list to trim.
     */
    static void Trim(std::vector<std::shared_ptr<CommandToken>>& args);
};

/**
 * @brief Quote token class.
 */
class CommandQuoteToken : public CommandToken
{
  public:
    /**
     * @brief Constructor.
     * @param offset The offset.
     * @param quote_char The quote character.
     */
    CommandQuoteToken(int offset, char quote_char);

    /**
     * @brief Parses a quote token.
     * @param command_line The command line.
     * @param index The current index (will be updated).
     * @return The parsed quote token.
     */
    static std::shared_ptr<CommandQuoteToken> Parse(const std::string& command_line, int& index);

    /**
     * @brief Gets the quote character.
     * @return The quote character.
     */
    char GetQuoteChar() const;

  private:
    char quote_char_;
};

/**
 * @brief String token class.
 */
class CommandStringToken : public CommandToken
{
  public:
    /**
     * @brief Constructor.
     * @param offset The offset.
     * @param value The string value.
     */
    CommandStringToken(int offset, const std::string& value);

    /**
     * @brief Parses a string token.
     * @param command_line The command line.
     * @param index The current index (will be updated).
     * @param quote The current quote context.
     * @return The parsed string token, or nullptr if none.
     */
    static std::shared_ptr<CommandStringToken> Parse(const std::string& command_line, int& index,
                                                     std::shared_ptr<CommandQuoteToken> quote = nullptr);
};
}  // namespace neo::console_service
