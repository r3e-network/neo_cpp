/**
 * @file jstring.cpp
 * @brief Jstring
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/json/jstring.h>

#include <iomanip>
#include <sstream>

namespace neo::json
{
JString::JString(const std::string& value) : value_(value) {}

JString::JString(std::string&& value) : value_(std::move(value)) {}

JTokenType JString::GetType() const { return JTokenType::String; }

std::string JString::AsString() const { return value_; }

std::string JString::GetString() const { return value_; }

std::string JString::ToString() const { return "\"" + EscapeString(value_) + "\""; }

std::shared_ptr<JToken> JString::Clone() const { return std::make_shared<JString>(value_); }

bool JString::Equals(const JToken& other) const
{
    if (other.GetType() != JTokenType::String) return false;

    const auto& other_string = static_cast<const JString&>(other);
    return value_ == other_string.value_;
}

JString::operator std::string() const { return value_; }

const std::string& JString::GetValue() const { return value_; }

void JString::WriteJson(std::string& output, bool indented, int indent_level) const { output += ToString(); }

std::string JString::EscapeString(const std::string& str)
{
    std::ostringstream oss;
    for (char c : str)
    {
        switch (c)
        {
            case '"':
                oss << "\\\"";
                break;
            case '\\':
                oss << "\\\\";
                break;
            case '\b':
                oss << "\\b";
                break;
            case '\f':
                oss << "\\f";
                break;
            case '\n':
                oss << "\\n";
                break;
            case '\r':
                oss << "\\r";
                break;
            case '\t':
                oss << "\\t";
                break;
            default:
                if (c >= 0 && c < 0x20)
                {
                    oss << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(c);
                }
                else
                {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}
}  // namespace neo::json
