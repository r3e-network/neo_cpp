/**
 * @file jtoken.cpp
 * @brief Jtoken
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/json/jarray.h>
#include <neo/json/jboolean.h>
#include <neo/json/jnumber.h>
#include <neo/json/jobject.h>
#include <neo/json/jstring.h>
#include <neo/json/jtoken.h>

#include <nlohmann/json.hpp>
#include <sstream>

namespace neo::json
{
// Static null token
const std::shared_ptr<JToken> JToken::Null = nullptr;

std::shared_ptr<JToken> JToken::Parse(const std::string& json, int max_nest)
{
    try
    {
        nlohmann::json j = nlohmann::json::parse(json);
        return ParseJsonValue(j);
    }
    catch (const nlohmann::json::exception& e)
    {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }
}

std::shared_ptr<JToken> JToken::ParseJsonValue(const nlohmann::json& j)
{
    switch (j.type())
    {
        case nlohmann::json::value_t::null:
            return Null;
        case nlohmann::json::value_t::boolean:
            return std::make_shared<JBoolean>(j.get<bool>());
        case nlohmann::json::value_t::number_integer:
        case nlohmann::json::value_t::number_unsigned:
        case nlohmann::json::value_t::number_float:
            return std::make_shared<JNumber>(j.get<double>());
        case nlohmann::json::value_t::string:
            return std::make_shared<JString>(j.get<std::string>());
        case nlohmann::json::value_t::array:
        {
            auto array = std::make_shared<JArray>();
            for (const auto& item : j)
            {
                array->Add(ParseJsonValue(item));
            }
            return array;
        }
        case nlohmann::json::value_t::object:
        {
            auto object = std::make_shared<JObject>();
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                object->SetProperty(it.key(), ParseJsonValue(it.value()));
            }
            return object;
        }
        default:
            throw std::runtime_error("Unsupported JSON value type");
    }
}

std::string JToken::ToString(bool indented) const
{
    std::string output;
    WriteJson(output, indented, 0);
    return output;
}

JToken::operator bool() const { return AsBoolean(); }

JToken::operator std::string() const { return AsString(); }

JToken::operator int() const { return GetInt32(); }

JToken::operator double() const { return AsNumber(); }

void JToken::AddIndentation(std::string& output, int indent_level)
{
    for (int i = 0; i < indent_level; ++i)
    {
        output += "  ";
    }
}
}  // namespace neo::json
