#include <neo/cryptography/ecc/ec_point.h>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace neo::cryptography
{
    // PIMPL implementation
    class ECPoint::Impl
    {
    public:
        std::vector<uint8_t> encodedPoint_;
        bool isInfinity_;

        Impl() : isInfinity_(true) {}
        
        explicit Impl(const std::vector<uint8_t>& encoded) 
            : encodedPoint_(encoded), isInfinity_(encoded.empty()) {}
    };

    ECPoint::ECPoint() : pImpl_(std::make_unique<Impl>()) {}

    ECPoint::ECPoint(const std::vector<uint8_t>& encodedPoint) 
        : pImpl_(std::make_unique<Impl>(encodedPoint)) {}

    ECPoint::ECPoint(const std::string& hexString) : pImpl_(std::make_unique<Impl>())
    {
        *this = Parse(hexString);
    }

    ECPoint::ECPoint(const ECPoint& other) : pImpl_(std::make_unique<Impl>(*other.pImpl_)) {}

    ECPoint& ECPoint::operator=(const ECPoint& other)
    {
        if (this != &other)
        {
            *pImpl_ = *other.pImpl_;
        }
        return *this;
    }

    ECPoint::~ECPoint() = default;

    ECPoint ECPoint::Parse(const std::string& hexString)
    {
        if (hexString.empty())
        {
            return ECPoint(); // Point at infinity
        }

        // Remove "0x" prefix if present
        std::string cleanHex = hexString;
        if (cleanHex.length() >= 2 && cleanHex.substr(0, 2) == "0x")
        {
            cleanHex = cleanHex.substr(2);
        }

        // Validate hex string length (should be even)
        if (cleanHex.length() % 2 != 0)
        {
            throw std::invalid_argument("Invalid hex string length");
        }

        // Validate hex characters
        for (char c : cleanHex)
        {
            if (!std::isxdigit(c))
            {
                throw std::invalid_argument("Invalid hex character in string");
            }
        }

        // Convert hex string to bytes
        std::vector<uint8_t> bytes;
        bytes.reserve(cleanHex.length() / 2);

        for (size_t i = 0; i < cleanHex.length(); i += 2)
        {
            std::string byteStr = cleanHex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            bytes.push_back(byte);
        }

        // Basic validation for EC point format
        if (bytes.size() < 33 || bytes.size() > 65)
        {
            throw std::invalid_argument("Invalid EC point length");
        }

        // Check compression flag
        uint8_t compressionFlag = bytes[0];
        if (compressionFlag != 0x02 && compressionFlag != 0x03 && compressionFlag != 0x04)
        {
            throw std::invalid_argument("Invalid EC point compression flag");
        }

        // Validate length based on compression
        if ((compressionFlag == 0x02 || compressionFlag == 0x03) && bytes.size() != 33)
        {
            throw std::invalid_argument("Invalid compressed EC point length");
        }
        else if (compressionFlag == 0x04 && bytes.size() != 65)
        {
            throw std::invalid_argument("Invalid uncompressed EC point length");
        }

        return ECPoint(bytes);
    }

    std::optional<ECPoint> ECPoint::FromBytes(const std::vector<uint8_t>& data)
    {
        try
        {
            // Basic validation for EC point format
            if (data.size() < 33 || data.size() > 65)
            {
                return std::nullopt;
            }

            // Check compression flag
            uint8_t compressionFlag = data[0];
            if (compressionFlag != 0x02 && compressionFlag != 0x03 && compressionFlag != 0x04)
            {
                return std::nullopt;
            }

            // Validate length based on compression
            if ((compressionFlag == 0x02 || compressionFlag == 0x03) && data.size() != 33)
            {
                return std::nullopt;
            }
            else if (compressionFlag == 0x04 && data.size() != 65)
            {
                return std::nullopt;
            }

            ECPoint point(data);
            if (!point.IsValid())
            {
                return std::nullopt;
            }

            return point;
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    std::optional<ECPoint> ECPoint::FromBytes(const neo::io::ByteSpan& span)
    {
        std::vector<uint8_t> data(span.Data(), span.Data() + span.Size());
        return FromBytes(data);
    }

    std::vector<uint8_t> ECPoint::GetEncoded() const
    {
        return pImpl_->encodedPoint_;
    }

    std::string ECPoint::ToString() const
    {
        if (IsInfinity())
        {
            return "";
        }

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        
        for (uint8_t byte : pImpl_->encodedPoint_)
        {
            oss << std::setw(2) << static_cast<unsigned>(byte);
        }
        
        return oss.str();
    }

    bool ECPoint::IsInfinity() const
    {
        return pImpl_->isInfinity_;
    }

    bool ECPoint::IsValid() const
    {
        if (IsInfinity())
        {
            return true;
        }

        const auto& encoded = pImpl_->encodedPoint_;
        if (encoded.empty())
        {
            return false;
        }

        uint8_t compressionFlag = encoded[0];
        
        // Check valid compression flags
        if (compressionFlag != 0x02 && compressionFlag != 0x03 && compressionFlag != 0x04)
        {
            return false;
        }

        // Check length
        if ((compressionFlag == 0x02 || compressionFlag == 0x03) && encoded.size() != 33)
        {
            return false;
        }
        else if (compressionFlag == 0x04 && encoded.size() != 65)
        {
            return false;
        }

        // Additional validation could be added here for curve equation
        return true;
    }

    bool ECPoint::operator==(const ECPoint& other) const
    {
        if (IsInfinity() && other.IsInfinity())
        {
            return true;
        }
        
        if (IsInfinity() || other.IsInfinity())
        {
            return false;
        }

        return pImpl_->encodedPoint_ == other.pImpl_->encodedPoint_;
    }

    bool ECPoint::operator!=(const ECPoint& other) const
    {
        return !(*this == other);
    }

    bool ECPoint::operator<(const ECPoint& other) const
    {
        if (IsInfinity() && other.IsInfinity())
        {
            return false;
        }
        
        if (IsInfinity())
        {
            return true;
        }
        
        if (other.IsInfinity())
        {
            return false;
        }

        return pImpl_->encodedPoint_ < other.pImpl_->encodedPoint_;
    }

    size_t ECPoint::Size() const
    {
        return pImpl_->encodedPoint_.size();
    }
}

// Hash function implementation
namespace std
{
    size_t hash<neo::cryptography::ECPoint>::operator()(const neo::cryptography::ECPoint& point) const
    {
        if (point.IsInfinity())
        {
            return 0;
        }

        const auto& encoded = point.GetEncoded();
        size_t hash = 0;
        
        for (uint8_t byte : encoded)
        {
            hash ^= std::hash<uint8_t>{}(byte) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }
        
        return hash;
    }
} 