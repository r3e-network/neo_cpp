#include <algorithm>
#include <cstring>
#include <neo/cryptography/base58.h>
#include <neo/cryptography/hash.h>
#include <stdexcept>

namespace neo::cryptography
{
std::string Base58::Encode(const std::vector<uint8_t>& data)
{
    if (data.empty())
    {
        return "";
    }

    // Count leading zeros
    size_t leadingZeros = 0;
    for (uint8_t byte : data)
    {
        if (byte == 0)
        {
            leadingZeros++;
        }
        else
        {
            break;
        }
    }

    // Convert to base 58
    std::vector<uint8_t> temp(data.begin() + leadingZeros, data.end());
    std::vector<uint8_t> result;

    while (!temp.empty())
    {
        uint32_t remainder = 0;
        for (size_t i = 0; i < temp.size(); ++i)
        {
            uint32_t value = remainder * 256 + temp[i];
            temp[i] = static_cast<uint8_t>(value / 58);
            remainder = value % 58;
        }

        result.push_back(static_cast<uint8_t>(remainder));

        // Remove leading zeros
        while (!temp.empty() && temp[0] == 0)
        {
            temp.erase(temp.begin());
        }
    }

    // Add leading '1's for leading zeros
    std::string encoded(leadingZeros, '1');

    // Convert result to string (reverse order)
    for (auto it = result.rbegin(); it != result.rend(); ++it)
    {
        encoded += ALPHABET[*it];
    }

    return encoded;
}

std::string Base58::Encode(const neo::io::ByteVector& data)
{
    std::vector<uint8_t> vec(data.begin(), data.end());
    return Encode(vec);
}

std::string Base58::Encode(const neo::io::ByteSpan& data)
{
    std::vector<uint8_t> vec(data.Data(), data.Data() + data.Size());
    return Encode(vec);
}

std::vector<uint8_t> Base58::Decode(const std::string& encoded)
{
    if (encoded.empty())
    {
        return {};
    }

    // Count leading '1's
    size_t leadingOnes = 0;
    for (char c : encoded)
    {
        if (c == '1')
        {
            leadingOnes++;
        }
        else
        {
            break;
        }
    }

    // Convert from base 58
    std::vector<uint8_t> result;
    for (size_t i = leadingOnes; i < encoded.size(); ++i)
    {
        char c = encoded[i];
        const char* pos = std::strchr(ALPHABET, c);
        if (!pos)
        {
            throw std::runtime_error("Invalid Base58 character");
        }

        uint8_t digit = static_cast<uint8_t>(pos - ALPHABET);

        // Multiply result by 58 and add digit
        uint32_t carry = digit;
        for (size_t j = 0; j < result.size(); ++j)
        {
            carry += result[j] * 58;
            result[j] = carry & 0xFF;
            carry >>= 8;
        }

        while (carry > 0)
        {
            result.push_back(carry & 0xFF);
            carry >>= 8;
        }
    }

    // Add leading zeros for leading '1's
    std::vector<uint8_t> output(leadingOnes, 0);

    // Reverse and append result
    for (auto it = result.rbegin(); it != result.rend(); ++it)
    {
        output.push_back(*it);
    }

    return output;
}

neo::io::ByteVector Base58::DecodeToByteVector(const std::string& encoded)
{
    auto vec = Decode(encoded);
    return neo::io::ByteVector(vec);
}

std::string Base58::EncodeCheck(const std::vector<uint8_t>& data)
{
    auto checksum = CalculateChecksum(data);
    std::vector<uint8_t> dataWithChecksum = data;
    dataWithChecksum.insert(dataWithChecksum.end(), checksum.begin(), checksum.end());
    return Encode(dataWithChecksum);
}

std::string Base58::EncodeCheck(const neo::io::ByteVector& data)
{
    std::vector<uint8_t> vec(data.begin(), data.end());
    return EncodeCheck(vec);
}

std::string Base58::EncodeCheck(const neo::io::ByteSpan& data)
{
    std::vector<uint8_t> vec(data.Data(), data.Data() + data.Size());
    return EncodeCheck(vec);
}

std::vector<uint8_t> Base58::DecodeCheck(const std::string& encoded)
{
    auto decoded = Decode(encoded);
    if (decoded.size() < 4)
    {
        throw std::runtime_error("Invalid Base58Check data: too short");
    }

    // Split data and checksum
    std::vector<uint8_t> data(decoded.begin(), decoded.end() - 4);
    std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());

    // Verify checksum
    auto expectedChecksum = CalculateChecksum(data);
    if (checksum != expectedChecksum)
    {
        throw std::runtime_error("Invalid Base58Check checksum");
    }

    return data;
}

neo::io::ByteVector Base58::DecodeCheckToByteVector(const std::string& encoded)
{
    auto vec = DecodeCheck(encoded);
    return neo::io::ByteVector(vec);
}

bool Base58::IsValid(const std::string& str)
{
    try
    {
        Decode(str);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool Base58::IsValidCheck(const std::string& str)
{
    try
    {
        DecodeCheck(str);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::vector<uint8_t> Base58::CalculateChecksum(const std::vector<uint8_t>& data)
{
    // Calculate SHA256(SHA256(data))
    io::ByteSpan dataSpan(data.data(), data.size());
    auto hash1 = Hash::Sha256(dataSpan);
    auto hash2 = Hash::Sha256(hash1.AsSpan());

    // Return first 4 bytes
    std::vector<uint8_t> result(4);
    std::memcpy(result.data(), hash2.Data(), 4);
    return result;
}
}  // namespace neo::cryptography