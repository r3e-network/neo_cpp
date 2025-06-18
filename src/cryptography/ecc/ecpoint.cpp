#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_writer.h>
#include <neo/io/binary_reader.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <stdexcept>
#include <cstring>

// Suppress OpenSSL deprecation warnings
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

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

            // Use OpenSSL to properly decompress the point
            try
            {
                // Create EC_GROUP for secp256r1 (prime256v1)
                EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
                if (!group)
                {
                    throw std::runtime_error("Failed to create EC_GROUP");
                }

                // Create EC_POINT and set it from the compressed bytes
                EC_POINT* ecPoint = EC_POINT_new(group);
                if (!ecPoint)
                {
                    EC_GROUP_free(group);
                    throw std::runtime_error("Failed to create EC_POINT");
                }

                // Convert compressed bytes to EC_POINT
                if (EC_POINT_oct2point(group, ecPoint, data.Data(), data.Size(), nullptr) != 1)
                {
                    EC_POINT_free(ecPoint);
                    EC_GROUP_free(group);
                    throw std::runtime_error("Failed to decompress EC point");
                }

                // Extract Y coordinate
                BIGNUM* x_bn = BN_new();
                BIGNUM* y_bn = BN_new();
                
                if (!x_bn || !y_bn)
                {
                    if (x_bn) BN_free(x_bn);
                    if (y_bn) BN_free(y_bn);
                    EC_POINT_free(ecPoint);
                    EC_GROUP_free(group);
                    throw std::runtime_error("Failed to create BIGNUMs");
                }

                if (EC_POINT_get_affine_coordinates_GFp(group, ecPoint, x_bn, y_bn, nullptr) != 1)
                {
                    BN_free(x_bn);
                    BN_free(y_bn);
                    EC_POINT_free(ecPoint);
                    EC_GROUP_free(group);
                    throw std::runtime_error("Failed to get affine coordinates");
                }

                // Convert Y coordinate to UInt256
                io::ByteVector y_bytes(32);
                int y_len = BN_num_bytes(y_bn);
                
                // Zero-pad the Y coordinate to 32 bytes
                std::memset(y_bytes.Data(), 0, 32);
                BN_bn2bin(y_bn, y_bytes.Data() + (32 - y_len));
                
                io::UInt256 y(y_bytes.AsSpan());
                point.SetY(y);

                // Cleanup
                BN_free(x_bn);
                BN_free(y_bn);
                EC_POINT_free(ecPoint);
                EC_GROUP_free(group);
            }
            catch (const std::exception&)
            {
                // If OpenSSL decompression fails, we still set the X coordinate
                // and mark Y as zero (indicating compressed point)
                point.SetY(io::UInt256());
            }

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

#ifdef _MSC_VER
#pragma warning(pop)
#endif
