/**
 * @file json_writer.cpp
 * @brief JSON serialization utilities
 */

#include <neo/io/ijson_serializable.h>
#include <neo/io/json_writer.h>

namespace neo::io
{
namespace
{
inline nlohmann::json& select_target(nlohmann::json& root, nlohmann::json* currentObject)
{
    return currentObject != nullptr ? *currentObject : root;
}
}  // namespace

JsonWriter::JsonWriter() : json_(ownedJson_)
{
    ownedJson_ = nlohmann::json::object();
}

JsonWriter::JsonWriter(nlohmann::json& json) : json_(json) {}

void JsonWriter::Write(const std::string& key, bool value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, uint8_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, uint16_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, uint32_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, uint64_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, int8_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, int16_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, int32_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, int64_t value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, const std::string& value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, const ByteSpan& value)
{
    select_target(json_, currentObject_)[key] = value.ToHexString();
}

void JsonWriter::Write(const std::string& key, const UInt160& value)
{
    select_target(json_, currentObject_)[key] = value.ToHexString();
}

void JsonWriter::Write(const std::string& key, const UInt256& value)
{
    select_target(json_, currentObject_)[key] = value.ToHexString();
}

void JsonWriter::Write(const std::string& key, const Fixed8& value)
{
    select_target(json_, currentObject_)[key] = value.ToString();
}

void JsonWriter::Write(const std::string& key, const nlohmann::json& value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::Write(const std::string& key, const IJsonSerializable& value)
{
    nlohmann::json obj = nlohmann::json::object();
    JsonWriter writer(obj);
    value.SerializeJson(writer);
    select_target(json_, currentObject_)[key] = std::move(obj);
}

void JsonWriter::WriteBase64String(const std::string& key, const ByteSpan& value)
{
    select_target(json_, currentObject_)[key] = value.ToBase64String();
}

void JsonWriter::WriteString(const std::string& key, const std::string& value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::WriteNumber(const std::string& key, double value)
{
    select_target(json_, currentObject_)[key] = value;
}

void JsonWriter::WriteString(const std::string& value)
{
    if (!currentPropertyName_.empty())
    {
        select_target(json_, currentObject_)[currentPropertyName_] = value;
        currentPropertyName_.clear();
    }
    else if (currentArray_ != nullptr)
    {
        currentArray_->push_back(value);
    }
    else if (currentObject_ != nullptr)
    {
        throw std::runtime_error("WriteString called without property name in object context");
    }
}

void JsonWriter::WriteNumber(double value)
{
    if (!currentPropertyName_.empty())
    {
        select_target(json_, currentObject_)[currentPropertyName_] = value;
        currentPropertyName_.clear();
    }
    else if (currentArray_ != nullptr)
    {
        currentArray_->push_back(value);
    }
    else if (currentObject_ != nullptr)
    {
        throw std::runtime_error("WriteNumber called without property name in object context");
    }
}

void JsonWriter::WriteNumber(int value)
{
    WriteNumber(static_cast<double>(value));
}

void JsonWriter::WriteStartArray()
{
    if (!currentPropertyName_.empty())
    {
        auto& target = select_target(json_, currentObject_);
        target[currentPropertyName_] = nlohmann::json::array();
        currentArray_ = &target[currentPropertyName_];
        currentPropertyName_.clear();
    }
    else if (currentArray_ != nullptr)
    {
        currentArray_->push_back(nlohmann::json::array());
        currentArray_ = &currentArray_->back();
    }
    else
    {
        json_ = nlohmann::json::array();
        currentArray_ = &json_;
    }
    currentObject_ = nullptr;
}

void JsonWriter::WriteEndArray()
{
    currentArray_ = nullptr;
}

void JsonWriter::WriteStartObject()
{
    if (currentArray_ != nullptr && currentPropertyName_.empty())
    {
        currentArray_->push_back(nlohmann::json::object());
        currentObject_ = &currentArray_->back();
    }
    else
    {
        auto& target = select_target(json_, currentObject_);
        if (!currentPropertyName_.empty())
        {
            target[currentPropertyName_] = nlohmann::json::object();
            currentObject_ = &target[currentPropertyName_];
            currentPropertyName_.clear();
        }
        else
        {
            json_ = nlohmann::json::object();
            currentObject_ = &json_;
        }
    }
}

void JsonWriter::WriteEndObject()
{
    currentObject_ = nullptr;
}

}  // namespace neo::io
