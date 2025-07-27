#include <algorithm>
#include <neo/cryptography/base64.h>
#include <neo/cryptography/base64url.h>

namespace neo::cryptography
{
std::string Base64Url::Encode(const io::ByteSpan& data)
{
    // Encode the data using Base64
    std::string base64 = Base64::Encode(data);

    // Replace '+' with '-'
    std::replace(base64.begin(), base64.end(), '+', '-');

    // Replace '/' with '_'
    std::replace(base64.begin(), base64.end(), '/', '_');

    // Remove padding '='
    base64.erase(std::remove(base64.begin(), base64.end(), '='), base64.end());

    return base64;
}

std::string Base64Url::Encode(const std::string& data)
{
    return Encode(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

io::ByteVector Base64Url::Decode(const std::string& data)
{
    // Create a copy of the data
    std::string base64 = data;

    // Replace '-' with '+'
    std::replace(base64.begin(), base64.end(), '-', '+');

    // Replace '_' with '/'
    std::replace(base64.begin(), base64.end(), '_', '/');

    // Add padding '='
    switch (base64.size() % 4)
    {
        case 0:
            break;
        case 2:
            base64 += "==";
            break;
        case 3:
            base64 += "=";
            break;
        default:
            throw std::runtime_error("Invalid Base64Url string");
    }

    // Decode the data using Base64
    return Base64::Decode(base64);
}

std::string Base64Url::DecodeToString(const std::string& data)
{
    auto bytes = Decode(data);
    return std::string(reinterpret_cast<const char*>(bytes.Data()), bytes.Size());
}
}  // namespace neo::cryptography
