#include <cmath>
#include <iomanip>
#include <neo/json/jnumber.h>
#include <sstream>

namespace neo::json
{
JNumber::JNumber(double value) : value_(value) {}

JTokenType JNumber::GetType() const
{
    return JTokenType::Number;
}

double JNumber::AsNumber() const
{
    return value_;
}

double JNumber::GetNumber() const
{
    return value_;
}

std::string JNumber::ToString() const
{
    if (std::isnan(value_))
        return "null";
    if (std::isinf(value_))
        return value_ > 0 ? "null" : "null";  // JSON doesn't support infinity

    // Check if it's an integer
    if (std::floor(value_) == value_ && value_ >= std::numeric_limits<long long>::min() &&
        value_ <= std::numeric_limits<long long>::max())
    {
        return std::to_string(static_cast<long long>(value_));
    }
    else
    {
        std::ostringstream oss;
        oss << std::setprecision(17) << value_;
        return oss.str();
    }
}

std::shared_ptr<JToken> JNumber::Clone() const
{
    return std::make_shared<JNumber>(value_);
}

bool JNumber::Equals(const JToken& other) const
{
    if (other.GetType() != JTokenType::Number)
        return false;

    const auto& other_number = static_cast<const JNumber&>(other);

    // Handle NaN comparison
    if (std::isnan(value_) && std::isnan(other_number.value_))
        return true;
    if (std::isnan(value_) || std::isnan(other_number.value_))
        return false;

    return value_ == other_number.value_;
}

JNumber::operator double() const
{
    return value_;
}

JNumber::operator int() const
{
    return static_cast<int>(value_);
}

double JNumber::GetValue() const
{
    return value_;
}

void JNumber::WriteJson(std::string& output, bool indented, int indent_level) const
{
    output += ToString();
}
}  // namespace neo::json
