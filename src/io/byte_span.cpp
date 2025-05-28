#include <neo/io/byte_span.h>
#include <sstream>
#include <iomanip>

namespace neo::io
{
    std::string ByteSpan::ToHexString() const
    {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        
        for (size_t i = 0; i < size_; i++)
        {
            oss << std::setw(2) << static_cast<int>(data_[i]);
        }
        
        return oss.str();
    }
}
