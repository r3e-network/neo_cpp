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
}
