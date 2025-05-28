#include <neo/io/fixed8.h>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace neo::io
{
    std::string Fixed8::ToString() const
    {
        int64_t absValue = std::abs(value_);
        int64_t intPart = absValue / Decimals;
        int64_t fracPart = absValue % Decimals;
        
        std::ostringstream oss;
        if (value_ < 0)
            oss << '-';
        
        oss << intPart;
        
        if (fracPart > 0)
        {
            oss << '.';
            
            // Convert to string with leading zeros
            std::ostringstream fracOss;
            fracOss << std::setw(8) << std::setfill('0') << fracPart;
            std::string fracStr = fracOss.str();
            
            // Remove trailing zeros
            while (!fracStr.empty() && fracStr.back() == '0')
                fracStr.pop_back();
            
            oss << fracStr;
        }
        
        return oss.str();
    }
}
