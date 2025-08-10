#include <neo/json/jobject.h>

namespace neo::json
{
JObject::JObject() {}

JObject::JObject(const Properties& properties) : properties_(properties) {}

JTokenType JObject::GetType() const { return JTokenType::Object; }

std::shared_ptr<JToken> JObject::operator[](const std::string& key) const
{
    if (properties_.contains(key))
    {
        return properties_.at(key);
    }
    return nullptr;
}

std::shared_ptr<JToken> JObject::operator[](const char* key) const { return operator[](std::string(key)); }

std::string JObject::ToString() const
{
    std::string output;
    WriteJson(output, false, 0);
    return output;
}

std::shared_ptr<JToken> JObject::Clone() const
{
    auto cloned = std::make_shared<JObject>();
    for (size_t i = 0; i < properties_.size(); ++i)
    {
        const auto& key = properties_.key_at(i);
        const auto& value = properties_.value_at(i);
        cloned->SetProperty(key, value ? value->Clone() : nullptr);
    }
    return cloned;
}

bool JObject::Equals(const JToken& other) const
{
    if (other.GetType() != JTokenType::Object) return false;

    const auto& other_object = static_cast<const JObject&>(other);
    if (properties_.size() != other_object.properties_.size()) return false;

    for (size_t i = 0; i < properties_.size(); ++i)
    {
        const auto& key = properties_.key_at(i);
        const auto& value1 = properties_.value_at(i);

        if (!other_object.properties_.contains(key)) return false;

        const auto& value2 = other_object.properties_.at(key);

        if (value1 == nullptr && value2 == nullptr) continue;
        if (value1 == nullptr || value2 == nullptr) return false;
        if (!value1->Equals(*value2)) return false;
    }

    return true;
}

void JObject::SetProperty(const std::string& key, std::shared_ptr<JToken> value)
{
    properties_.insert_or_assign(key, value);
}

const JObject::Properties& JObject::GetProperties() const { return properties_; }

JObject::Properties& JObject::GetProperties() { return properties_; }

bool JObject::ContainsProperty(const std::string& key) const { return properties_.contains(key); }

void JObject::Clear() { properties_.clear(); }

size_t JObject::Count() const { return properties_.size(); }

void JObject::WriteJson(std::string& output, bool indented, int indent_level) const
{
    output += "{";

    if (indented && !properties_.empty())
    {
        output += "\n";
    }

    for (size_t i = 0; i < properties_.size(); ++i)
    {
        if (indented)
        {
            AddIndentation(output, indent_level + 1);
        }

        const auto& key = properties_.key_at(i);
        const auto& value = properties_.value_at(i);

        // Write key
        output += "\"" + key + "\":";
        if (indented)
        {
            output += " ";
        }

        // Write value
        if (value)
        {
            value->WriteJson(output, indented, indent_level + 1);
        }
        else
        {
            output += "null";
        }

        if (i < properties_.size() - 1)
        {
            output += ",";
        }

        if (indented)
        {
            output += "\n";
        }
    }

    if (indented && !properties_.empty())
    {
        AddIndentation(output, indent_level);
    }

    output += "}";
}
}  // namespace neo::json
