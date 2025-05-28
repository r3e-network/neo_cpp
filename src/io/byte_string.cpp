#include <neo/io/byte_string.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace neo {
namespace io {

ByteString::ByteString() = default;

ByteString::ByteString(const std::vector<uint8_t>& data)
    : data_(data) {
}

ByteString::ByteString(std::vector<uint8_t>&& data)
    : data_(std::move(data)) {
}

ByteString::ByteString(const uint8_t* data, size_t size)
    : data_(data, data + size) {
}

ByteString::ByteString(const std::string& hex) {
    FromHexString(hex);
}

ByteString::ByteString(const char* hex)
    : ByteString(std::string(hex)) {
}

ByteString::ByteString(const ByteString& other)
    : data_(other.data_) {
}

ByteString::ByteString(ByteString&& other) noexcept
    : data_(std::move(other.data_)) {
}

ByteString& ByteString::operator=(const ByteString& other) {
    if (this != &other) {
        data_ = other.data_;
    }
    return *this;
}

ByteString& ByteString::operator=(ByteString&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
    }
    return *this;
}

bool ByteString::operator==(const ByteString& other) const {
    return data_ == other.data_;
}

bool ByteString::operator!=(const ByteString& other) const {
    return !(*this == other);
}

bool ByteString::operator<(const ByteString& other) const {
    return data_ < other.data_;
}

uint8_t& ByteString::operator[](size_t index) {
    return data_[index];
}

const uint8_t& ByteString::operator[](size_t index) const {
    return data_[index];
}

ByteString ByteString::operator+(const ByteString& other) const {
    ByteString result(*this);
    result.data_.insert(result.data_.end(), other.data_.begin(), other.data_.end());
    return result;
}

ByteString& ByteString::operator+=(const ByteString& other) {
    data_.insert(data_.end(), other.data_.begin(), other.data_.end());
    return *this;
}

const std::vector<uint8_t>& ByteString::GetData() const {
    return data_;
}

std::vector<uint8_t>& ByteString::GetData() {
    return data_;
}

size_t ByteString::GetSize() const {
    return data_.size();
}

bool ByteString::IsEmpty() const {
    return data_.empty();
}

void ByteString::Clear() {
    data_.clear();
}

void ByteString::Resize(size_t size) {
    data_.resize(size);
}

void ByteString::Reserve(size_t size) {
    data_.reserve(size);
}

void ByteString::PushBack(uint8_t value) {
    data_.push_back(value);
}

void ByteString::PopBack() {
    data_.pop_back();
}

void ByteString::Insert(size_t pos, const ByteString& other) {
    data_.insert(data_.begin() + pos, other.data_.begin(), other.data_.end());
}

void ByteString::Erase(size_t pos, size_t count) {
    data_.erase(data_.begin() + pos, data_.begin() + pos + count);
}

ByteString ByteString::Substring(size_t pos, size_t count) const {
    if (pos >= data_.size()) {
        return ByteString();
    }
    
    size_t end = (count == std::string::npos) ? data_.size() : std::min(pos + count, data_.size());
    return ByteString(data_.data() + pos, end - pos);
}

std::string ByteString::ToHexString() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (const auto& byte : data_) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    
    return ss.str();
}

void ByteString::FromHexString(const std::string& hex) {
    data_.clear();
    
    // Skip "0x" prefix if present
    size_t start = 0;
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) {
        start = 2;
    }
    
    // Ensure even number of characters
    size_t length = hex.size() - start;
    if (length % 2 != 0) {
        throw std::invalid_argument("Hex string must have an even number of characters");
    }
    
    data_.reserve(length / 2);
    
    for (size_t i = start; i < hex.size(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        try {
            uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
            data_.push_back(byte);
        } catch (const std::exception& e) {
            throw std::invalid_argument("Invalid hex character in string");
        }
    }
}

} // namespace io
} // namespace neo
