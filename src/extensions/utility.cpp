#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <iomanip>
#include <limits>
#include <neo/extensions/utility.h>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#endif

namespace neo::extensions
{
std::vector<std::string> Utility::Split(const std::string& str, const std::string& delimiter, bool removeEmpty)
{
    std::vector<std::string> result;

    if (str.empty() || delimiter.empty())
    {
        if (!str.empty() || !removeEmpty)
            result.push_back(str);
        return result;
    }

    size_t start = 0;
    size_t pos = 0;

    while ((pos = str.find(delimiter, start)) != std::string::npos)
    {
        std::string part = str.substr(start, pos - start);
        if (!part.empty() || !removeEmpty)
            result.push_back(part);
        start = pos + delimiter.length();
    }

    // Add the last part
    std::string lastPart = str.substr(start);
    if (!lastPart.empty() || !removeEmpty)
        result.push_back(lastPart);

    return result;
}

std::string Utility::Join(const std::vector<std::string>& parts, const std::string& delimiter)
{
    if (parts.empty())
        return "";

    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            oss << delimiter;
        oss << parts[i];
    }

    return oss.str();
}

std::string Utility::Trim(const std::string& str)
{
    return TrimRight(TrimLeft(str));
}

std::string Utility::TrimLeft(const std::string& str)
{
    auto start = str.find_first_not_of(" \t\n\r\f\v");
    return (start == std::string::npos) ? "" : str.substr(start);
}

std::string Utility::TrimRight(const std::string& str)
{
    auto end = str.find_last_not_of(" \t\n\r\f\v");
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string Utility::ToLower(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string Utility::ToUpper(const std::string& str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
    return result;
}

bool Utility::StartsWith(const std::string& str, const std::string& prefix, bool ignoreCase)
{
    if (prefix.length() > str.length())
        return false;

    if (ignoreCase)
    {
        return ToLower(str.substr(0, prefix.length())) == ToLower(prefix);
    }
    else
    {
        return str.substr(0, prefix.length()) == prefix;
    }
}

bool Utility::EndsWith(const std::string& str, const std::string& suffix, bool ignoreCase)
{
    if (suffix.length() > str.length())
        return false;

    if (ignoreCase)
    {
        return ToLower(str.substr(str.length() - suffix.length())) == ToLower(suffix);
    }
    else
    {
        return str.substr(str.length() - suffix.length()) == suffix;
    }
}

bool Utility::Contains(const std::string& str, const std::string& substring, bool ignoreCase)
{
    if (ignoreCase)
    {
        return ToLower(str).find(ToLower(substring)) != std::string::npos;
    }
    else
    {
        return str.find(substring) != std::string::npos;
    }
}

std::string Utility::Replace(const std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty())
        return str;

    std::string result = str;
    size_t pos = 0;

    while ((pos = result.find(from, pos)) != std::string::npos)
    {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }

    return result;
}

void Utility::SecureZeroMemory(void* ptr, size_t size)
{
    if (!ptr || size == 0)
        return;

    // Use volatile to prevent compiler optimization
    volatile uint8_t* volatilePtr = static_cast<volatile uint8_t*>(ptr);
    for (size_t i = 0; i < size; ++i)
    {
        volatilePtr[i] = 0;
    }

    // Memory barrier to prevent reordering (compiler-specific)
#ifdef _WIN32
    _ReadWriteBarrier();
#elif defined(__GNUC__)
    __asm__ __volatile__("" ::: "memory");
#endif
}

bool Utility::SecureCompareMemory(const void* a, const void* b, size_t size)
{
    if (!a || !b)
        return false;

    const uint8_t* ptr1 = static_cast<const uint8_t*>(a);
    const uint8_t* ptr2 = static_cast<const uint8_t*>(b);

    // Constant-time comparison to prevent timing attacks
    uint8_t result = 0;
    for (size_t i = 0; i < size; ++i)
    {
        result |= ptr1[i] ^ ptr2[i];
    }

    return result == 0;
}

uint32_t Utility::NextPowerOf2(uint32_t value)
{
    if (value == 0)
        return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;

    return value;
}

bool Utility::IsPowerOf2(uint32_t value)
{
    return value != 0 && (value & (value - 1)) == 0;
}

void Utility::ReverseBytes(uint8_t* data, size_t size)
{
    if (!data || size <= 1)
        return;

    for (size_t i = 0; i < size / 2; ++i)
    {
        std::swap(data[i], data[size - 1 - i]);
    }
}

std::string Utility::BytesToHex(const io::ByteSpan& data, bool uppercase)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    if (uppercase)
        oss << std::uppercase;

    for (size_t i = 0; i < data.Size(); ++i)
    {
        oss << std::setw(2) << static_cast<unsigned int>(data[i]);
    }

    return oss.str();
}

io::ByteVector Utility::HexToBytes(const std::string& hex)
{
    if (hex.length() % 2 != 0)
        throw std::invalid_argument("Hex string length must be even");

    io::ByteVector result;
    result.Reserve(hex.length() / 2);

    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoul(byteString, nullptr, 16));
        result.Push(byte);
    }

    return result;
}

// Template specializations for TryParse
template <>
bool Utility::TryParse<int>(const std::string& str, int& result)
{
    try
    {
        size_t pos;
        result = std::stoi(str, &pos);
        return pos == str.length();
    }
    catch (const std::invalid_argument& e)
    {
        // Invalid string format for numeric conversion
        return false;
    }
    catch (const std::out_of_range& e)
    {
        // Number out of range for target type
        return false;
    }
}

template <>
bool Utility::TryParse<long>(const std::string& str, long& result)
{
    try
    {
        size_t pos;
        result = std::stol(str, &pos);
        return pos == str.length();
    }
    catch (const std::invalid_argument& e)
    {
        // Invalid string format for numeric conversion
        return false;
    }
    catch (const std::out_of_range& e)
    {
        // Number out of range for target type
        return false;
    }
}

template <>
bool Utility::TryParse<long long>(const std::string& str, long long& result)
{
    try
    {
        size_t pos;
        result = std::stoll(str, &pos);
        return pos == str.length();
    }
    catch (const std::invalid_argument& e)
    {
        // Invalid string format for numeric conversion
        return false;
    }
    catch (const std::out_of_range& e)
    {
        // Number out of range for target type
        return false;
    }
}

template <>
bool Utility::TryParse<unsigned int>(const std::string& str, unsigned int& result)
{
    try
    {
        size_t pos;
        unsigned long parsedValue = std::stoul(str, &pos);
        if (pos != str.length() || parsedValue > std::numeric_limits<unsigned int>::max())
            return false;
        result = static_cast<unsigned int>(parsedValue);
        return true;
    }
    catch (const std::invalid_argument& e)
    {
        // Invalid string format for numeric conversion
        return false;
    }
    catch (const std::out_of_range& e)
    {
        // Number out of range for target type
        return false;
    }
}

template <>
bool Utility::TryParse<float>(const std::string& str, float& result)
{
    try
    {
        size_t pos;
        result = std::stof(str, &pos);
        return pos == str.length();
    }
    catch (const std::invalid_argument& e)
    {
        // Invalid string format for numeric conversion
        return false;
    }
    catch (const std::out_of_range& e)
    {
        // Number out of range for target type
        return false;
    }
}

template <>
bool Utility::TryParse<double>(const std::string& str, double& result)
{
    try
    {
        size_t pos;
        result = std::stod(str, &pos);
        return pos == str.length();
    }
    catch (const std::invalid_argument& e)
    {
        // Invalid string format for numeric conversion
        return false;
    }
    catch (const std::out_of_range& e)
    {
        // Number out of range for target type
        return false;
    }
}

// Template specializations for Parse
template <>
int Utility::Parse<int>(const std::string& str)
{
    int result;
    if (!TryParse(str, result))
        throw std::invalid_argument("Cannot parse string to int: " + str);
    return result;
}

template <>
long Utility::Parse<long>(const std::string& str)
{
    long result;
    if (!TryParse(str, result))
        throw std::invalid_argument("Cannot parse string to long: " + str);
    return result;
}

template <>
float Utility::Parse<float>(const std::string& str)
{
    float result;
    if (!TryParse(str, result))
        throw std::invalid_argument("Cannot parse string to float: " + str);
    return result;
}

template <>
double Utility::Parse<double>(const std::string& str)
{
    double result;
    if (!TryParse(str, result))
        throw std::invalid_argument("Cannot parse string to double: " + str);
    return result;
}

// Template specializations for ToString
template <>
std::string Utility::ToString<int>(const int& value)
{
    return std::to_string(value);
}

template <>
std::string Utility::ToString<long>(const long& value)
{
    return std::to_string(value);
}

template <>
std::string Utility::ToString<float>(const float& value)
{
    return std::to_string(value);
}

template <>
std::string Utility::ToString<double>(const double& value)
{
    return std::to_string(value);
}

// SafeCast specializations
template <>
int32_t Utility::SafeCast<int32_t, int64_t>(const int64_t& value)
{
    if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max())
        throw std::overflow_error("Value out of range for int32_t");
    return static_cast<int32_t>(value);
}

template <>
uint32_t Utility::SafeCast<uint32_t, uint64_t>(const uint64_t& value)
{
    if (value > std::numeric_limits<uint32_t>::max())
        throw std::overflow_error("Value out of range for uint32_t");
    return static_cast<uint32_t>(value);
}

// Format implementation
template <>
std::string Utility::Format<>(const std::string& format)
{
    return format;
}

}  // namespace neo::extensions
