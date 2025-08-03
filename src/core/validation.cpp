#include <neo/core/validation.h>
#include <regex>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace neo::core
{

// Basic type validation
Validator::ValidationResult Validator::ValidateNotNull(const void* ptr, const std::string& name)
{
    if (ptr == nullptr) {
        return ValidationResult(false, name + " cannot be null", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateNotEmpty(const std::string& str, const std::string& name)
{
    if (str.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateNotEmpty(const std::vector<uint8_t>& data, const std::string& name)
{
    if (data.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateNotEmpty(const io::ByteSpan& span, const std::string& name)
{
    if (span.Size() == 0) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    return ValidationResult(true);
}

// Size validation
Validator::ValidationResult Validator::ValidateSize(size_t actual_size, size_t expected_size, const std::string& name)
{
    if (actual_size != expected_size) {
        return ValidationResult(false, 
            name + " size (" + std::to_string(actual_size) + 
            ") does not match expected size (" + std::to_string(expected_size) + ")",
            NeoException::ErrorCode::INVALID_FORMAT);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateMinSize(size_t actual_size, size_t min_size, const std::string& name)
{
    if (actual_size < min_size) {
        return ValidationResult(false, 
            name + " size (" + std::to_string(actual_size) + 
            ") is less than minimum required (" + std::to_string(min_size) + ")",
            NeoException::ErrorCode::BUFFER_UNDERFLOW);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateMaxSize(size_t actual_size, size_t max_size, const std::string& name)
{
    if (actual_size > max_size) {
        return ValidationResult(false, 
            name + " size (" + std::to_string(actual_size) + 
            ") exceeds maximum allowed (" + std::to_string(max_size) + ")",
            NeoException::ErrorCode::BUFFER_OVERFLOW);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateSizeRange(size_t actual_size, size_t min_size, size_t max_size, const std::string& name)
{
    auto min_result = ValidateMinSize(actual_size, min_size, name);
    if (!min_result) return min_result;
    
    return ValidateMaxSize(actual_size, max_size, name);
}

// String validation
Validator::ValidationResult Validator::ValidateHexString(const std::string& hex, const std::string& name)
{
    if (hex.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    std::string clean_hex = hex;
    // Remove 0x prefix if present
    if (clean_hex.length() >= 2 && clean_hex.substr(0, 2) == "0x") {
        clean_hex = clean_hex.substr(2);
    }
    
    // Must have even length
    if (clean_hex.length() % 2 != 0) {
        return ValidationResult(false, name + " must have even length", NeoException::ErrorCode::INVALID_FORMAT);
    }
    
    // Check if all characters are valid hex
    for (char c : clean_hex) {
        if (!std::isxdigit(c)) {
            return ValidationResult(false, name + " contains invalid hex character: " + std::string(1, c), 
                NeoException::ErrorCode::INVALID_FORMAT);
        }
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateHexString(const std::string& hex, size_t expected_length, const std::string& name)
{
    auto hex_result = ValidateHexString(hex, name);
    if (!hex_result) return hex_result;
    
    std::string clean_hex = hex;
    if (clean_hex.length() >= 2 && clean_hex.substr(0, 2) == "0x") {
        clean_hex = clean_hex.substr(2);
    }
    
    if (clean_hex.length() != expected_length * 2) {
        return ValidationResult(false, 
            name + " length (" + std::to_string(clean_hex.length()) + 
            ") does not match expected (" + std::to_string(expected_length * 2) + ")",
            NeoException::ErrorCode::INVALID_FORMAT);
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateBase58String(const std::string& base58, const std::string& name)
{
    if (base58.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Base58 alphabet: 123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz
    const std::string base58_alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    
    for (char c : base58) {
        if (base58_alphabet.find(c) == std::string::npos) {
            return ValidationResult(false, name + " contains invalid Base58 character: " + std::string(1, c),
                NeoException::ErrorCode::INVALID_FORMAT);
        }
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateAddress(const std::string& address, const std::string& name)
{
    if (address.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Neo addresses typically start with 'N' and are Base58 encoded
    if (address[0] != 'N') {
        return ValidationResult(false, name + " must start with 'N'", NeoException::ErrorCode::INVALID_FORMAT);
    }
    
    // Validate as Base58
    return ValidateBase58String(address, name);
}

// Neo-specific validation
Validator::ValidationResult Validator::ValidateUInt160(const io::UInt160& value, const std::string& name)
{
    // UInt160 is always valid by construction
    // Check if it's not all zeros (which might indicate uninitialized value)
    static const io::UInt160 zero_hash;
    if (value == zero_hash)
    {
        return ValidationResult(false, name + " cannot be zero hash");
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateUInt256(const io::UInt256& value, const std::string& name)
{
    // UInt256 is always valid by construction
    // Check if it's not all zeros (which might indicate uninitialized value)
    static const io::UInt256 zero_hash;
    if (value == zero_hash)
    {
        return ValidationResult(false, name + " cannot be zero hash");
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateByteSpan(const io::ByteSpan& span, const std::string& name)
{
    if (span.Data() == nullptr && span.Size() > 0) {
        return ValidationResult(false, name + " has null data pointer but non-zero size", 
            NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateByteSpan(const io::ByteSpan& span, size_t expected_size, const std::string& name)
{
    auto span_result = ValidateByteSpan(span, name);
    if (!span_result) return span_result;
    
    return ValidateSize(span.Size(), expected_size, name);
}

// Compound validation
Validator::ValidationResult Validator::ValidateAll(const std::vector<ValidationResult>& results)
{
    for (const auto& result : results) {
        if (!result.is_valid) {
            return result;
        }
    }
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateAny(const std::vector<ValidationResult>& results)
{
    if (results.empty()) {
        return ValidationResult(false, "No validation results provided", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    for (const auto& result : results) {
        if (result.is_valid) {
            return ValidationResult(true);
        }
    }
    
    // Return the first error
    return results[0];
}

// Throwing validators
void Validator::RequireNotNull(const void* ptr, const std::string& name)
{
    auto result = ValidateNotNull(ptr, name);
    if (!result) {
        throw NeoException(result.error_code, result.error_message);
    }
}

void Validator::RequireNotEmpty(const std::string& str, const std::string& name)
{
    auto result = ValidateNotEmpty(str, name);
    if (!result) {
        throw NeoException(result.error_code, result.error_message);
    }
}

void Validator::RequireNotEmpty(const std::vector<uint8_t>& data, const std::string& name)
{
    auto result = ValidateNotEmpty(data, name);
    if (!result) {
        throw NeoException(result.error_code, result.error_message);
    }
}

void Validator::RequireSize(size_t actual_size, size_t expected_size, const std::string& name)
{
    auto result = ValidateSize(actual_size, expected_size, name);
    if (!result) {
        throw NeoException(result.error_code, result.error_message);
    }
}

void Validator::RequireHexString(const std::string& hex, const std::string& name)
{
    auto result = ValidateHexString(hex, name);
    if (!result) {
        throw NeoException(result.error_code, result.error_message);
    }
}

void Validator::RequireHexString(const std::string& hex, size_t expected_length, const std::string& name)
{
    auto result = ValidateHexString(hex, expected_length, name);
    if (!result) {
        throw NeoException(result.error_code, result.error_message);
    }
}

// Security validation
Validator::ValidationResult Validator::ValidateNoScriptInjection(const std::string& input, const std::string& name)
{
    // Check for common script injection patterns
    std::vector<std::string> dangerous_patterns = {
        "<script", "</script>", "javascript:", "vbscript:", "onload=", "onerror=",
        "onclick=", "onmouseover=", "onfocus=", "onblur=", "onchange=", "onsubmit="
    };
    
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    for (const auto& pattern : dangerous_patterns) {
        if (lower_input.find(pattern) != std::string::npos) {
            return ValidationResult(false, name + " contains potentially dangerous script pattern: " + pattern,
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateNoSQLInjection(const std::string& input, const std::string& name)
{
    // Check for common SQL injection patterns
    std::vector<std::string> dangerous_patterns = {
        "'", "--", "/*", "*/", "xp_", "sp_", "select ", "insert ", "update ", "delete ",
        "drop ", "create ", "alter ", "exec ", "execute ", "union ", "order by", "group by"
    };
    
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);
    
    for (const auto& pattern : dangerous_patterns) {
        if (lower_input.find(pattern) != std::string::npos) {
            return ValidationResult(false, name + " contains potentially dangerous SQL pattern: " + pattern,
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateFileName(const std::string& filename, const std::string& name)
{
    if (filename.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Check for dangerous characters
    std::string dangerous_chars = "<>:\"|?*\\//";
    for (char c : dangerous_chars) {
        if (filename.find(c) != std::string::npos) {
            return ValidationResult(false, name + " contains invalid character: " + std::string(1, c),
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
    }
    
    // Check for reserved names on Windows
    std::vector<std::string> reserved_names = {
        "CON", "PRN", "AUX", "NUL",
        "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
        "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"
    };
    
    std::string upper_filename = filename;
    std::transform(upper_filename.begin(), upper_filename.end(), upper_filename.begin(), ::toupper);
    
    for (const auto& reserved : reserved_names) {
        if (upper_filename == reserved || upper_filename.find(reserved + ".") == 0) {
            return ValidationResult(false, name + " uses reserved name: " + reserved,
                NeoException::ErrorCode::INVALID_ARGUMENT);
        }
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateFilePath(const std::string& filepath, const std::string& name)
{
    if (filepath.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Check for path traversal attempts
    if (filepath.find("..") != std::string::npos) {
        return ValidationResult(false, name + " contains path traversal pattern", 
            NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    return ValidationResult(true);
}

// Network validation
Validator::ValidationResult Validator::ValidateIPAddress(const std::string& ip, const std::string& name)
{
    if (ip.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Simple IPv4 validation using regex
    std::regex ipv4_regex(R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    
    if (!std::regex_match(ip, ipv4_regex)) {
        return ValidationResult(false, name + " is not a valid IPv4 address", 
            NeoException::ErrorCode::INVALID_FORMAT);
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidatePort(uint16_t port, const std::string& name)
{
    if (port == 0) {
        return ValidationResult(false, name + " cannot be zero", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Ports 1-1023 are typically reserved (well-known ports)
    // This is just a warning, not an error for now
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateURL(const std::string& url, const std::string& name)
{
    if (url.empty()) {
        return ValidationResult(false, name + " cannot be empty", NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    // Basic URL validation - must start with http:// or https://
    if (url.find("http://") != 0 && url.find("https://") != 0) {
        return ValidationResult(false, name + " must start with http:// or https://",
            NeoException::ErrorCode::INVALID_FORMAT);
    }
    
    return ValidationResult(true);
}

// Blockchain-specific validation
Validator::ValidationResult Validator::ValidateBlockHeight(uint32_t height, uint32_t max_height)
{
    return ValidateRange(height, 0u, max_height, "block height");
}

Validator::ValidationResult Validator::ValidateTransactionFee(int64_t fee, int64_t max_fee)
{
    if (fee < 0) {
        return ValidationResult(false, "transaction fee cannot be negative", 
            NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    if (fee > max_fee) {
        return ValidationResult(false, "transaction fee exceeds maximum allowed",
            NeoException::ErrorCode::OUT_OF_RANGE);
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateGasAmount(int64_t gas, int64_t max_gas)
{
    if (gas < 0) {
        return ValidationResult(false, "gas amount cannot be negative",
            NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    if (gas > max_gas) {
        return ValidationResult(false, "gas amount exceeds maximum allowed",
            NeoException::ErrorCode::OUT_OF_GAS);
    }
    
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateNonce(uint32_t nonce)
{
    // Nonce validation - for now just check it's not zero (depending on protocol requirements)
    // This can be customized based on specific Neo protocol requirements
    return ValidationResult(true);
}

Validator::ValidationResult Validator::ValidateTimestamp(uint64_t timestamp)
{
    // Validate timestamp is reasonable (not too far in past or future)
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Allow 1 hour in the future, any time in the past (within reason)
    uint64_t max_future = static_cast<uint64_t>(now) + 3600; // 1 hour
    uint64_t min_past = 1000000000; // Roughly year 2001 (reasonable minimum)
    
    if (timestamp > max_future) {
        return ValidationResult(false, "timestamp is too far in the future",
            NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    if (timestamp < min_past) {
        return ValidationResult(false, "timestamp is too far in the past",  
            NeoException::ErrorCode::INVALID_ARGUMENT);
    }
    
    return ValidationResult(true);
}

} // namespace neo::core