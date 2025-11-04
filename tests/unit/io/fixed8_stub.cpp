#include <neo/core/fixed8.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace neo::core
{
std::string Fixed8::ToString() const
{
    double val = static_cast<double>(value_) / SCALE_FACTOR;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(8) << val;
    std::string result = oss.str();
    auto pos = result.find_last_not_of('0');
    if (pos != std::string::npos)
    {
        if (result[pos] == '.') pos--;
        result.erase(pos + 1);
    }
    return result;
}

Fixed8 Fixed8::Parse(const std::string& str)
{
    if (str.empty()) throw std::invalid_argument("Cannot parse empty Fixed8");

    std::string trimmed = str;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r\f\v"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r\f\v") + 1);
    if (trimmed.empty()) throw std::invalid_argument("Cannot parse whitespace Fixed8");

    size_t used = 0;
    double value = std::stod(trimmed, &used);
    if (used != trimmed.size() || !std::isfinite(value))
    {
        throw std::invalid_argument("Invalid Fixed8 format");
    }

    const double max_value = static_cast<double>(std::numeric_limits<int64_t>::max()) / SCALE_FACTOR;
    const double min_value = static_cast<double>(std::numeric_limits<int64_t>::min()) / SCALE_FACTOR;
    if (value > max_value || value < min_value)
    {
        throw std::overflow_error("Value exceeds Fixed8 range");
    }

    return Fixed8(value);
}
}  // namespace neo::core
