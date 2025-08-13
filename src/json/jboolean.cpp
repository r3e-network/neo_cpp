/**
 * @file jboolean.cpp
 * @brief Jboolean
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/json/jboolean.h>

namespace neo::json
{
JBoolean::JBoolean(bool value) : value_(value) {}

JTokenType JBoolean::GetType() const { return JTokenType::Boolean; }

bool JBoolean::AsBoolean() const { return value_; }

bool JBoolean::GetBoolean() const { return value_; }

std::string JBoolean::ToString() const { return value_ ? "true" : "false"; }

std::shared_ptr<JToken> JBoolean::Clone() const { return std::make_shared<JBoolean>(value_); }

bool JBoolean::Equals(const JToken& other) const
{
    if (other.GetType() != JTokenType::Boolean) return false;

    const auto& other_bool = static_cast<const JBoolean&>(other);
    return value_ == other_bool.value_;
}

JBoolean::operator bool() const { return value_; }

bool JBoolean::GetValue() const { return value_; }

void JBoolean::WriteJson(std::string& output, bool indented, int indent_level) const
{
    output += value_ ? "true" : "false";
}
}  // namespace neo::json
