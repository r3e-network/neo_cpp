#include <neo/io/ijson_serializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>

namespace neo::io
{
nlohmann::json IJsonSerializable::ToJson() const
{
    nlohmann::json json = nlohmann::json::object();
    JsonWriter writer(json);
    SerializeJson(writer);
    return json;
}

void IJsonSerializable::DeserializeFromJson(const nlohmann::json& json)
{
    JsonReader reader(json);
    DeserializeJson(reader);
}

std::string IJsonSerializable::ToJsonString(bool pretty) const
{
    nlohmann::json json = ToJson();
    if (pretty)
        return json.dump(4);
    else
        return json.dump();
}

void IJsonSerializable::DeserializeFromJsonString(const std::string& json)
{
    nlohmann::json jsonObj = nlohmann::json::parse(json);
    DeserializeFromJson(jsonObj);
}
}  // namespace neo::io
