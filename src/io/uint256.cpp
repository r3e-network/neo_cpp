#include <neo/io/uint256.h>
#include <neo/io/byte_vector.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <stdexcept>

namespace neo::io
{
    UInt256 UInt256::Parse(const std::string& hex)
    {
        UInt256 result;
        if (!TryParse(hex, result))
            throw std::invalid_argument("Invalid UInt256 format");
        return result;
    }

    bool UInt256::TryParse(const std::string& hex, UInt256& result)
    {
        try
        {
            // Remove '0x' prefix if present
            std::string hexStr = hex;
            if (hexStr.size() >= 2 && hexStr[0] == '0' && (hexStr[1] == 'x' || hexStr[1] == 'X'))
                hexStr = hexStr.substr(2);

            // Check length
            if (hexStr.length() != Size * 2)
                return false;

            // Validate hex characters
            for (char c : hexStr)
            {
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                    return false;
            }

            ByteVector bytes = ByteVector::Parse(hexStr);
            if (bytes.Size() != Size)
                return false;

            result = UInt256(bytes.AsSpan());
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool UInt256::IsZero() const
    {
        for (size_t i = 0; i < Size; i++)
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
