#include <neo/core/fixed8.h>
#include <sstream>
#include <iomanip>

namespace neo::core
{
    // Forward declaration implementation - will be defined when BigDecimal is available
    Fixed8::Fixed8(const BigDecimal& value) : value_(0)
    {
        // This will be implemented properly when BigDecimal is fully available
        // For now, just initialize to 0
    }

    std::string Fixed8::ToString() const
    {
        // Convert the raw scaled value to decimal representation
        double val = static_cast<double>(value_) / SCALE_FACTOR;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(8) << val;
        std::string result = oss.str();
        
        // Remove trailing zeros
        size_t end = result.find_last_not_of('0');
        if (end != std::string::npos && result[end] == '.')
            end--;
        result.erase(end + 1);
        
        return result;
    }

    Fixed8 Fixed8::Parse(const std::string& str)
    {
        try {
            double val = std::stod(str);
            return Fixed8(val);
        } catch (...) {
            // For now, just return zero if parsing fails
            return Fixed8();
        }
    }
}