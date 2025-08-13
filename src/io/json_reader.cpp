/**
 * @file json_reader.cpp
 * @brief JSON serialization utilities
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/io/ijson_serializable.h>
#include <neo/io/json_reader.h>

#include <stdexcept>

namespace neo::io
{
JsonReader::JsonReader(const nlohmann::json& json) : json_(json) {}

bool JsonReader::ReadBool(const std::string& key, bool defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_boolean()) return defaultValue;

    return json_[key].get<bool>();
}

uint8_t JsonReader::ReadUInt8(const std::string& key, uint8_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_unsigned()) return defaultValue;

    return json_[key].get<uint8_t>();
}

uint16_t JsonReader::ReadUInt16(const std::string& key, uint16_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_unsigned()) return defaultValue;

    return json_[key].get<uint16_t>();
}

uint32_t JsonReader::ReadUInt32(const std::string& key, uint32_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_unsigned()) return defaultValue;

    return json_[key].get<uint32_t>();
}

uint64_t JsonReader::ReadUInt64(const std::string& key, uint64_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_unsigned()) return defaultValue;

    return json_[key].get<uint64_t>();
}

int8_t JsonReader::ReadInt8(const std::string& key, int8_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_integer()) return defaultValue;

    return json_[key].get<int8_t>();
}

int16_t JsonReader::ReadInt16(const std::string& key, int16_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_integer()) return defaultValue;

    return json_[key].get<int16_t>();
}

int32_t JsonReader::ReadInt32(const std::string& key, int32_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_integer()) return defaultValue;

    return json_[key].get<int32_t>();
}

int64_t JsonReader::ReadInt64(const std::string& key, int64_t defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number_integer()) return defaultValue;

    return json_[key].get<int64_t>();
}

std::string JsonReader::ReadString(const std::string& key, const std::string& defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_string()) return defaultValue;

    return json_[key].get<std::string>();
}

ByteVector JsonReader::ReadBytes(const std::string& key) const
{
    if (!json_.contains(key) || !json_[key].is_string()) return ByteVector();

    std::string hex = json_[key].get<std::string>();
    return ByteVector::Parse(hex);
}

UInt160 JsonReader::ReadUInt160(const std::string& key) const
{
    if (!json_.contains(key) || !json_[key].is_string()) return UInt160::Zero();

    std::string hex = json_[key].get<std::string>();
    return UInt160::Parse(hex);
}

UInt256 JsonReader::ReadUInt256(const std::string& key) const
{
    if (!json_.contains(key) || !json_[key].is_string()) return UInt256::Zero();

    std::string hex = json_[key].get<std::string>();
    return UInt256::Parse(hex);
}

Fixed8 JsonReader::ReadFixed8(const std::string& key) const
{
    if (!json_.contains(key)) return Fixed8::Zero();

    if (json_[key].is_number())
    {
        double value = json_[key].get<double>();
        return Fixed8::FromDouble(value);
    }
    else if (json_[key].is_string())
    {
        std::string str = json_[key].get<std::string>();
        return Fixed8::Parse(str);
    }

    return Fixed8::Zero();
}

nlohmann::json JsonReader::ReadObject(const std::string& key) const
{
    if (!json_.contains(key) || !json_[key].is_object()) return nlohmann::json::object();

    return json_[key];
}

nlohmann::json JsonReader::ReadArray(const std::string& key) const
{
    if (!json_.contains(key) || !json_[key].is_array()) return nlohmann::json::array();

    return json_[key];
}

ByteVector JsonReader::ReadBase64String(const std::string& key) const
{
    if (!json_.contains(key) || !json_[key].is_string()) return ByteVector();

    std::string base64 = json_[key].get<std::string>();
    return ByteVector::FromBase64String(base64);
}

double JsonReader::ReadNumber(const std::string& key, double defaultValue) const
{
    if (!json_.contains(key) || !json_[key].is_number()) return defaultValue;

    return json_[key].get<double>();
}
}  // namespace neo::io
