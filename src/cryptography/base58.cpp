/**
 * @file base58.cpp
 * @brief Base58
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cryptography/base58.h>
#include <neo/cryptography/hash.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace neo::cryptography
{
std::string Base58::Encode(const std::vector<uint8_t>& data)
{
    if (data.empty()) return "";

    size_t leadingZeros = 0;
    while (leadingZeros < data.size() && data[leadingZeros] == 0) ++leadingZeros;

    if (leadingZeros == data.size()) return std::string(leadingZeros, '1');

    const size_t size = data.size() - leadingZeros;
    std::vector<uint8_t> b58(size * 138 / 100 + 1, 0);

    size_t length = 0;
    for (size_t i = leadingZeros; i < data.size(); ++i)
    {
        int carry = data[i];
        size_t j = 0;
        for (auto it = b58.rbegin(); (carry != 0 || j < length) && it != b58.rend(); ++it, ++j)
        {
            carry += 256 * (*it);
            *it = static_cast<uint8_t>(carry % 58);
            carry /= 58;
        }
        length = j;
    }

    auto it = b58.begin() + (b58.size() - length);
    while (it != b58.end() && *it == 0) ++it;

    std::string encoded(leadingZeros, '1');
    for (; it != b58.end(); ++it)
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
    if (encoded.empty()) return {};

    size_t leadingOnes = 0;
    while (leadingOnes < encoded.size() && encoded[leadingOnes] == '1') ++leadingOnes;

    if (leadingOnes == encoded.size()) return std::vector<uint8_t>(leadingOnes, 0);

    const size_t size = encoded.size() - leadingOnes;
    std::vector<uint8_t> b256(size * 733 / 1000 + 1, 0);

    size_t length = 0;
    for (size_t i = leadingOnes; i < encoded.size(); ++i)
    {
        const char* pos = std::strchr(ALPHABET, encoded[i]);
        if (!pos)
        {
            throw std::invalid_argument("Invalid Base58 character");
        }

        int carry = static_cast<int>(pos - ALPHABET);
        size_t j = 0;
        for (auto it = b256.rbegin(); (carry != 0 || j < length) && it != b256.rend(); ++it, ++j)
        {
            carry += 58 * (*it);
            *it = static_cast<uint8_t>(carry & 0xFF);
            carry >>= 8;
        }
        length = j;
    }

    auto it = b256.begin() + (b256.size() - length);
    while (it != b256.end() && *it == 0) ++it;

    std::vector<uint8_t> output;
    output.reserve(leadingOnes + static_cast<size_t>(std::distance(it, b256.end())));
    output.insert(output.end(), leadingOnes, 0);
    for (; it != b256.end(); ++it) output.push_back(*it);
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
        throw std::invalid_argument("Invalid Base58Check data");
    }

    std::vector<uint8_t> data(decoded.begin(), decoded.end() - 4);
    std::vector<uint8_t> checksum(decoded.end() - 4, decoded.end());

    auto expectedChecksum = CalculateChecksum(data);
    if (checksum != expectedChecksum)
    {
        throw std::invalid_argument("Invalid Base58Check checksum");
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
    catch (const std::invalid_argument&)
    {
        return false;
    }
    catch (const std::runtime_error&)
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
    catch (const std::invalid_argument&)
    {
        return false;
    }
    catch (const std::runtime_error&)
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
