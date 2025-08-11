#pragma once

#include <neo/io/byte_vector.h>

#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace neo::io
{
/**
 * @brief JSON handling interface for Neo C++ node.
 *
 * This provides a wrapper around nlohmann/json with Neo-specific
 * functionality for blockchain data serialization/deserialization.
 */
class JsonValue
{
   public:
    using json = nlohmann::json;

    // Constructors
    JsonValue() : data_(json::object()) {}
    JsonValue(const json& j) : data_(j) {}
    JsonValue(json&& j) : data_(std::move(j)) {}
    JsonValue(const std::string& str) : data_(json::parse(str)) {}
    
    // Copy constructor - don't copy cached elements
    JsonValue(const JsonValue& other) : data_(other.data_) {}
    
    // Move constructor
    JsonValue(JsonValue&& other) noexcept 
        : data_(std::move(other.data_)), 
          cached_elements_(std::move(other.cached_elements_)) {}
    
    // Copy assignment
    JsonValue& operator=(const JsonValue& other) {
        if (this != &other) {
            data_ = other.data_;
            cached_elements_.clear();  // Clear cache on copy
        }
        return *this;
    }
    
    // Move assignment
    JsonValue& operator=(JsonValue&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            cached_elements_ = std::move(other.cached_elements_);
        }
        return *this;
    }

    // Type checks
    bool IsNull() const { return data_.is_null(); }
    bool IsBoolean() const { return data_.is_boolean(); }
    bool IsNumber() const { return data_.is_number(); }
    bool IsString() const { return data_.is_string(); }
    bool IsArray() const { return data_.is_array(); }
    bool IsObject() const { return data_.is_object(); }

    // Value getters
    bool GetBoolean() const { return data_.get<bool>(); }
    int32_t GetInt32() const { return data_.get<int32_t>(); }
    int64_t GetInt64() const { return data_.get<int64_t>(); }
    uint32_t GetUInt32() const { return data_.get<uint32_t>(); }
    uint64_t GetUInt64() const { return data_.get<uint64_t>(); }
    double GetDouble() const { return data_.get<double>(); }
    std::string GetString() const { return data_.get<std::string>(); }

    // Array operations
    size_t Size() const { return data_.size(); }
    JsonValue operator[](size_t index) const { return JsonValue(data_[index]); }
    JsonValue& operator[](size_t index)
    {
        // Complete array element access implementation with proper reference handling
        if (!data_.is_array())
        {
            throw std::runtime_error("JsonValue is not an array");
        }

        // Ensure the array is large enough
        while (index >= data_.size())
        {
            // Add null elements until we reach the desired index
            data_.push_back(nullptr);
        }

        // Return a JsonValue that wraps a reference to the actual array element
        // This approach maintains proper reference semantics while avoiding thread_local issues

        // Use placement new to create a JsonValue wrapper that maintains reference to the element
        // Create a reference wrapper that shares the same underlying json object
        if (!cached_elements_[index])
        {
            cached_elements_[index] = std::make_unique<JsonValue>(data_[index]);
        }
        return *cached_elements_[index];
    }

    // Object operations
    JsonValue operator[](const std::string& key) const
    {
        auto it = data_.find(key);
        return it != data_.end() ? JsonValue(*it) : JsonValue(json::value_t::null);
    }

    bool HasMember(const std::string& key) const { return data_.contains(key); }

    // Value setters
    void SetBoolean(bool value) { data_ = value; }
    void SetInt32(int32_t value) { data_ = value; }
    void SetInt64(int64_t value) { data_ = value; }
    void SetUInt32(uint32_t value) { data_ = value; }
    void SetUInt64(uint64_t value) { data_ = value; }
    void SetDouble(double value) { data_ = value; }
    void SetString(const std::string& value) { data_ = value; }
    void SetNull() { data_ = json::value_t::null; }

    // Object member operations
    void AddMember(const std::string& key, const JsonValue& value) { data_[key] = value.data_; }

    void AddMember(const std::string& key, bool value) { data_[key] = value; }
    void AddMember(const std::string& key, int32_t value) { data_[key] = value; }
    void AddMember(const std::string& key, int64_t value) { data_[key] = value; }
    void AddMember(const std::string& key, uint32_t value) { data_[key] = value; }
    void AddMember(const std::string& key, uint64_t value) { data_[key] = value; }
    void AddMember(const std::string& key, double value) { data_[key] = value; }
    void AddMember(const std::string& key, const std::string& value) { data_[key] = value; }
    void AddMember(const std::string& key, const char* value) { data_[key] = std::string(value); }

    // Array operations
    void PushBack(const JsonValue& value) { data_.push_back(value.data_); }
    void PushBack(bool value) { data_.push_back(value); }
    void PushBack(int32_t value) { data_.push_back(value); }
    void PushBack(int64_t value) { data_.push_back(value); }
    void PushBack(uint32_t value) { data_.push_back(value); }
    void PushBack(uint64_t value) { data_.push_back(value); }
    void PushBack(double value) { data_.push_back(value); }
    void PushBack(const std::string& value) { data_.push_back(value); }

    // Serialization
    std::string ToString() const { return data_.dump(); }
    std::string ToString(int indent) const { return data_.dump(indent); }

    // Neo-specific helpers
    void AddByteArray(const std::string& key, const ByteVector& bytes)
    {
        std::string hex = "0x";
        for (uint8_t byte : bytes)
        {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", byte);
            hex += buf;
        }
        data_[key] = hex;
    }

    ByteVector GetByteArray(const std::string& key) const
    {
        std::string hex = data_[key].get<std::string>();
        if (hex.substr(0, 2) == "0x")
        {
            hex = hex.substr(2);
        }

        ByteVector result;
        for (size_t i = 0; i < hex.length(); i += 2)
        {
            std::string byteStr = hex.substr(i, 2);
            uint8_t byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
            result.Push(byte);
        }
        return result;
    }

    // Access to underlying json
    const json& GetJson() const { return data_; }
    json& GetJson() { return data_; }

    // Static factory methods
    static JsonValue CreateObject() { return JsonValue(json::object()); }
    static JsonValue CreateArray() { return JsonValue(json::array()); }
    static JsonValue Parse(const std::string& str) { return JsonValue(json::parse(str)); }

   private:
    json data_;
    mutable std::unordered_map<size_t, std::unique_ptr<JsonValue>>
        cached_elements_;  // Cache for array element references
};

// Type aliases for compatibility
using Json = JsonValue;
using JsonObject = JsonValue;
using JsonArray = JsonValue;

// Utility functions
inline std::string ToJsonString(const JsonValue& value, int indent = -1)
{
    return indent >= 0 ? value.ToString(indent) : value.ToString();
}

inline JsonValue ParseJson(const std::string& str) { return JsonValue::Parse(str); }
}  // namespace neo::io