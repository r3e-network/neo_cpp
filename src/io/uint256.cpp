#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace neo::io
{
    UInt256 UInt256::Parse(const std::string& hex)
    {
        UInt256 result;
        if (!TryParse(hex, result))
        {
            throw std::invalid_argument("Invalid UInt256 hex string: " + hex);
        }
        return result;
    }

    bool UInt256::TryParse(const std::string& hex, UInt256& result)
    {
        std::string cleanHex = hex;
        
        // Remove 0x prefix if present
        if (cleanHex.size() >= 2 && cleanHex.substr(0, 2) == "0x")
        {
            cleanHex = cleanHex.substr(2);
        }
        
        // Pad with leading zeros if necessary
        if (cleanHex.size() < Size * 2)
        {
            cleanHex = std::string(Size * 2 - cleanHex.size(), '0') + cleanHex;
        }
        
        // Check length
        if (cleanHex.size() != Size * 2)
        {
            return false;
        }
        
        // Parse hex string
        for (size_t i = 0; i < Size; ++i)
        {
            std::string byteStr = cleanHex.substr(i * 2, 2);
            char* endPtr;
            unsigned long byteVal = std::strtoul(byteStr.c_str(), &endPtr, 16);
            
            if (*endPtr != '\0' || byteVal > 255)
            {
                return false;
            }
            
            result.data_[i] = static_cast<uint8_t>(byteVal);
        }
        
        return true;
    }

    UInt256 UInt256::FromString(const std::string& hex_string)
    {
        return Parse(hex_string);
    }

    UInt256 UInt256::FromLittleEndianString(const std::string& hex_string)
    {
        std::string cleanHex = hex_string;
        
        // Remove 0x prefix if present
        if (cleanHex.size() >= 2 && cleanHex.substr(0, 2) == "0x")
        {
            cleanHex = cleanHex.substr(2);
        }
        
        // Pad with leading zeros if necessary
        if (cleanHex.size() < Size * 2)
        {
            cleanHex = std::string(Size * 2 - cleanHex.size(), '0') + cleanHex;
        }
        
        UInt256 result;
        
        // Parse hex string in little-endian order
        for (size_t i = 0; i < Size; ++i)
        {
            std::string byteStr = cleanHex.substr(i * 2, 2);
            char* endPtr;
            unsigned long byteVal = std::strtoul(byteStr.c_str(), &endPtr, 16);
            
            if (*endPtr != '\0' || byteVal > 255)
            {
                throw std::invalid_argument("Invalid hex string: " + hex_string);
            }
            
            result.data_[i] = static_cast<uint8_t>(byteVal);
        }
        
        return result;
    }

    std::string UInt256::ToString(bool reverse) const
    {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        
        if (reverse)
        {
            // Little-endian output (reverse byte order)
            for (int i = Size - 1; i >= 0; --i)
            {
                ss << std::setw(2) << static_cast<unsigned>(data_[i]);
            }
        }
        else
        {
            // Big-endian output
            for (size_t i = 0; i < Size; ++i)
            {
                ss << std::setw(2) << static_cast<unsigned>(data_[i]);
            }
        }
        
        return ss.str();
    }

    std::string UInt256::ToLittleEndianString() const
    {
        return ToString(false);
    }

    bool UInt256::IsZero() const
    {
        for (size_t i = 0; i < Size; ++i)
        {
            if (data_[i] != 0)
                return false;
        }
        return true;
    }

    void UInt256::Serialize(BinaryWriter& writer) const
    {
        writer.WriteBytes(data_.data(), Size);
    }

    void UInt256::Deserialize(BinaryReader& reader)
    {
        reader.ReadBytes(data_.data(), Size);
    }
}
