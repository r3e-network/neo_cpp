#include <neo/io/json_writer.h>
#include <neo/io/ijson_serializable.h>

namespace neo::io
{
    JsonWriter::JsonWriter() : json_(ownedJson_)
    {
        ownedJson_ = nlohmann::json::object();
    }

    JsonWriter::JsonWriter(nlohmann::json& json) : json_(json)
    {
    }

    void JsonWriter::Write(const std::string& key, bool value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, uint8_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, uint16_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, uint32_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, uint64_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, int8_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, int16_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, int32_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, int64_t value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, const std::string& value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, const ByteSpan& value)
    {
        json_[key] = value.ToHexString();
    }

    void JsonWriter::Write(const std::string& key, const UInt160& value)
    {
        json_[key] = value.ToHexString();
    }

    void JsonWriter::Write(const std::string& key, const UInt256& value)
    {
        json_[key] = value.ToHexString();
    }

    void JsonWriter::Write(const std::string& key, const Fixed8& value)
    {
        json_[key] = value.ToString();
    }

    void JsonWriter::Write(const std::string& key, const nlohmann::json& value)
    {
        json_[key] = value;
    }

    void JsonWriter::Write(const std::string& key, const IJsonSerializable& value)
    {
        nlohmann::json obj = nlohmann::json::object();
        JsonWriter writer(obj);
        value.SerializeJson(writer);
        json_[key] = obj;
    }

    void JsonWriter::WriteBase64String(const std::string& key, const ByteSpan& value)
    {
        json_[key] = value.ToBase64String();
    }

    void JsonWriter::WriteString(const std::string& key, const std::string& value)
    {
        json_[key] = value;
    }

    void JsonWriter::WriteNumber(const std::string& key, double value)
    {
        json_[key] = value;
    }

    void JsonWriter::WriteString(const std::string& value)
    {
        if (!currentPropertyName_.empty())
        {
            json_[currentPropertyName_] = value;
            currentPropertyName_.clear();
        }
        else if (currentArray_ != nullptr)
        {
            currentArray_->push_back(value);
        }
        else if (currentObject_ != nullptr)
        {
            // Can't write string without property name in object context
            throw std::runtime_error("WriteString called without property name in object context");
        }
    }

    void JsonWriter::WriteNumber(double value)
    {
        if (!currentPropertyName_.empty())
        {
            json_[currentPropertyName_] = value;
            currentPropertyName_.clear();
        }
        else if (currentArray_ != nullptr)
        {
            currentArray_->push_back(value);
        }
        else if (currentObject_ != nullptr)
        {
            // Can't write number without property name in object context
            throw std::runtime_error("WriteNumber called without property name in object context");
        }
    }

    void JsonWriter::WriteNumber(int value)
    {
        WriteNumber(static_cast<double>(value));
    }
}
