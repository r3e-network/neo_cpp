#include <neo/io/byte_span.h>
#include <sstream>
#include <iomanip>

namespace neo::io
{
    std::string ByteSpan::ToHexString() const
    {
        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0');
        
        for (size_t i = 0; i < size_; i++)
        {
            oss << std::setw(2) << static_cast<int>(data_[i]);
        }
        
        return oss.str();
    }

    std::string ByteSpan::ToBase64String() const
    {
        // Base64 encoding characters
        static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz0123456789+/";
        
        std::string result;
        result.reserve(((size_ + 2) / 3) * 4);
        
        for (size_t i = 0; i < size_; i += 3)
        {
            uint32_t triple = 0;
            
            // First byte
            triple |= (data_[i] << 16);
            
            // Second byte (if exists)
            if (i + 1 < size_)
                triple |= (data_[i + 1] << 8);
            
            // Third byte (if exists)
            if (i + 2 < size_)
                triple |= data_[i + 2];
            
            // Extract 6-bit chunks and encode
            result += alphabet[(triple >> 18) & 0x3F];
            result += alphabet[(triple >> 12) & 0x3F];
            result += (i + 1 < size_) ? alphabet[(triple >> 6) & 0x3F] : '=';
            result += (i + 2 < size_) ? alphabet[triple & 0x3F] : '=';
        }
        
        return result;
    }
}
