#include <neo/core/fixed8.h>
#include <neo/core/big_decimal.h>
#include <sstream>
#include <iomanip>

namespace neo::core
{
    // Complete BigDecimal to Fixed8 conversion implementation
    // NOTE: Temporarily disabled to avoid circular dependency issues
    /*Fixed8::Fixed8(const BigDecimal& value) : value_(0)
    {
        try {
            // Complete BigDecimal to Fixed8 conversion using native BigDecimal methods
            // Use BigDecimal's native ToDouble() method for efficient conversion
            double decimal_value = value.ToDouble();*/
            
            // Validate range - Fixed8 uses int64_t with 8 decimal places
            const double MAX_VALUE = static_cast<double>(std::numeric_limits<int64_t>::max()) / SCALE_FACTOR;
            const double MIN_VALUE = static_cast<double>(std::numeric_limits<int64_t>::min()) / SCALE_FACTOR;
            
            if (decimal_value > MAX_VALUE || decimal_value < MIN_VALUE) {
                throw std::overflow_error("BigDecimal value exceeds Fixed8 range");
            }
            
            // Convert to scaled integer with proper rounding
            value_ = static_cast<int64_t>(std::round(decimal_value * SCALE_FACTOR));
            
        } catch (const std::exception& e) {
            // BigDecimal conversion failed - this should not silently fail
            throw std::invalid_argument(std::string("Failed to convert BigDecimal to Fixed8: ") + e.what());
        }
    }*/

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
            // Validate input string
            if (str.empty()) {
                throw std::invalid_argument("Cannot parse empty string to Fixed8");
            }
            
            // Trim whitespace
            std::string trimmed = str;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r\f\v"));
            trimmed.erase(trimmed.find_last_not_of(" \t\n\r\f\v") + 1);
            
            if (trimmed.empty()) {
                throw std::invalid_argument("Cannot parse whitespace-only string to Fixed8");
            }
            
            // Parse the decimal value
            double val = std::stod(trimmed);
            
            // Validate that the conversion used the entire string
            size_t pos = 0;
            std::stod(trimmed, &pos);
            if (pos != trimmed.length()) {
                throw std::invalid_argument("Invalid Fixed8 format: contains non-numeric characters");
            }
            
            // Check for special values
            if (!std::isfinite(val)) {
                throw std::invalid_argument("Fixed8 cannot represent infinite or NaN values");
            }
            
            // Validate range
            const double MAX_VALUE = static_cast<double>(std::numeric_limits<int64_t>::max()) / SCALE_FACTOR;
            const double MIN_VALUE = static_cast<double>(std::numeric_limits<int64_t>::min()) / SCALE_FACTOR;
            
            if (val > MAX_VALUE || val < MIN_VALUE) {
                throw std::overflow_error("Value exceeds Fixed8 range");
            }
            
            return Fixed8(val);
            
        } catch (const std::invalid_argument&) {
            // Re-throw invalid argument exceptions
            throw;
        } catch (const std::out_of_range&) {
            // std::stod throws out_of_range for values too large
            throw std::overflow_error("Value exceeds Fixed8 range");
        } catch (const std::exception& e) {
            // Any other parsing error
            throw std::invalid_argument(std::string("Failed to parse Fixed8: ") + e.what());
        }
    }
}