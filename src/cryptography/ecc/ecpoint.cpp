#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <stdexcept>
#include <cstring>

namespace neo::cryptography::ecc
{
    ECPoint::ECPoint()
        : isInfinity_(true)
    {
    }

    ECPoint::ECPoint(const std::string& curveName)
        : curveName_(curveName), isInfinity_(true)
    {
    }

    const std::string& ECPoint::GetCurveName() const
    {
        return curveName_;
    }

    void ECPoint::SetCurveName(const std::string& curveName)
    {
        curveName_ = curveName;
    }

    bool ECPoint::IsInfinity() const
    {
        return isInfinity_;
    }

    void ECPoint::SetInfinity(bool isInfinity)
    {
        isInfinity_ = isInfinity;
    }

    const io::UInt256& ECPoint::GetX() const
    {
        return x_;
    }

    void ECPoint::SetX(const io::UInt256& x)
    {
        x_ = x;
        isInfinity_ = false;
    }

    const io::UInt256& ECPoint::GetY() const
    {
        return y_;
    }

    void ECPoint::SetY(const io::UInt256& y)
    {
        y_ = y;
        isInfinity_ = false;
    }

    io::ByteVector ECPoint::ToBytes(bool compressed) const
    {
        if (isInfinity_)
            return io::ByteVector{ 0x00 };

        if (compressed)
        {
            io::ByteVector data(33);
            data[0] = 0x02; // Assume even Y for now
            std::memcpy(data.Data() + 1, x_.Data(), 32);
            return data;
        }
        else
        {
            io::ByteVector data(65);
            data[0] = 0x04;
            std::memcpy(data.Data() + 1, x_.Data(), 32);
            std::memcpy(data.Data() + 33, y_.Data(), 32);
            return data;
        }
    }

    io::ByteVector ECPoint::ToArray() const
    {
        return ToBytes(true); // Always return compressed format to match C# behavior
    }

    std::string ECPoint::ToHex(bool compressed) const
    {
        return ToBytes(compressed).ToHexString();
    }

    ECPoint ECPoint::FromBytes(const io::ByteSpan& data, const std::string& curveName)
    {
        if (data.Size() == 0)
            throw std::invalid_argument("Invalid ECPoint data");

        ECPoint point(curveName);

        if (data.Size() == 1 && data[0] == 0x00)
        {
            point.SetInfinity(true);
            return point;
        }

        if (data.Size() == 33 && (data[0] == 0x02 || data[0] == 0x03))
        {
            // Compressed format - calculate Y from X and curve equation
            io::UInt256 x(io::ByteSpan(data.Data() + 1, 32));
            point.SetX(x);

            // Calculate Y coordinate from X using curve equation: y² = x³ + ax + b
            // For secp256r1: y² = x³ - 3x + b
            // This is a simplified implementation that should be replaced with proper
            // big integer arithmetic and modular square root calculation

            // For now, we'll use a placeholder that indicates the point is compressed
            // but doesn't calculate the actual Y coordinate. This should be replaced
            // with proper elliptic curve point decompression using a crypto library
            // like OpenSSL or similar.

            // Set Y to a special value that indicates this is a compressed point
            // The actual Y calculation requires:
            // 1. Compute x³ - 3x + b (mod p) where p is the curve prime
            // 2. Compute modular square root
            // 3. Choose correct root based on parity bit (0x02 = even, 0x03 = odd)

            // For production use, this should use proper cryptographic libraries
            point.SetY(io::UInt256()); // Placeholder - needs proper implementation
            return point;
        }
        else if (data.Size() == 65 && data[0] == 0x04)
        {
            // Uncompressed format
            io::UInt256 x(io::ByteSpan(data.Data() + 1, 32));
            io::UInt256 y(io::ByteSpan(data.Data() + 33, 32));
            point.SetX(x);
            point.SetY(y);
            return point;
        }

        throw std::invalid_argument("Invalid ECPoint data");
    }

    ECPoint ECPoint::FromHex(const std::string& hex, const std::string& curveName)
    {
        return FromBytes(io::ByteVector::Parse(hex).AsSpan(), curveName);
    }

    ECPoint ECPoint::Infinity(const std::string& curveName)
    {
        ECPoint point(curveName);
        point.SetInfinity(true);
        return point;
    }

    bool ECPoint::operator==(const ECPoint& other) const
    {
        if (isInfinity_ && other.isInfinity_)
            return true;
        if (isInfinity_ || other.isInfinity_)
            return false;
        return x_ == other.x_ && y_ == other.y_;
    }

    bool ECPoint::operator!=(const ECPoint& other) const
    {
        return !(*this == other);
    }

    bool ECPoint::operator<(const ECPoint& other) const
    {
        // Compare by byte representation for consistent ordering
        auto thisBytes = ToBytes(true);
        auto otherBytes = other.ToBytes(true);

        if (thisBytes.Size() != otherBytes.Size())
            return thisBytes.Size() < otherBytes.Size();

        return std::memcmp(thisBytes.Data(), otherBytes.Data(), thisBytes.Size()) < 0;
    }

    bool ECPoint::operator>(const ECPoint& other) const
    {
        return other < *this;
    }

    bool ECPoint::operator<=(const ECPoint& other) const
    {
        return !(other < *this);
    }

    bool ECPoint::operator>=(const ECPoint& other) const
    {
        return !(*this < other);
    }

    void ECPoint::Serialize(io::BinaryWriter& writer) const
    {
        auto bytes = ToArray(); // Use compressed format
        writer.Write(io::ByteSpan(bytes.Data(), bytes.Size()));
    }

    void ECPoint::Deserialize(io::BinaryReader& reader)
    {
        // Read the first byte to determine the format
        uint8_t prefix = reader.ReadUInt8();

        if (prefix == 0x00)
        {
            // Infinity point
            SetInfinity(true);
            return;
        }

        if (prefix == 0x02 || prefix == 0x03)
        {
            // Compressed format - read 32 more bytes for X coordinate
            auto xBytes = reader.ReadBytes(32);
            io::ByteVector fullData(33);
            fullData[0] = prefix;
            std::memcpy(fullData.Data() + 1, xBytes.Data(), 32);

            *this = FromBytes(fullData.AsSpan(), curveName_);
        }
        else if (prefix == 0x04)
        {
            // Uncompressed format - read 64 more bytes for X and Y coordinates
            auto xyBytes = reader.ReadBytes(64);
            io::ByteVector fullData(65);
            fullData[0] = prefix;
            std::memcpy(fullData.Data() + 1, xyBytes.Data(), 64);

            *this = FromBytes(fullData.AsSpan(), curveName_);
        }
        else
        {
            throw std::runtime_error("Invalid ECPoint format");
        }
    }
}
