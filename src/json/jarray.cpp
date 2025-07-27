#include <neo/json/jarray.h>
#include <stdexcept>

namespace neo::json
{
JArray::JArray() {}

JArray::JArray(const Items& items) : items_(items) {}

JArray::JArray(std::initializer_list<std::shared_ptr<JToken>> items) : items_(items) {}

JTokenType JArray::GetType() const
{
    return JTokenType::Array;
}

std::shared_ptr<JToken> JArray::operator[](int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= items_.size())
        throw std::out_of_range("Array index out of range");
    return items_[index];
}

std::string JArray::ToString() const
{
    std::string output;
    WriteJson(output, false, 0);
    return output;
}

std::shared_ptr<JToken> JArray::Clone() const
{
    auto cloned = std::make_shared<JArray>();
    for (const auto& item : items_)
    {
        cloned->Add(item ? item->Clone() : nullptr);
    }
    return cloned;
}

bool JArray::Equals(const JToken& other) const
{
    if (other.GetType() != JTokenType::Array)
        return false;

    const auto& other_array = static_cast<const JArray&>(other);
    if (items_.size() != other_array.items_.size())
        return false;

    for (size_t i = 0; i < items_.size(); ++i)
    {
        const auto& item1 = items_[i];
        const auto& item2 = other_array.items_[i];

        if (item1 == nullptr && item2 == nullptr)
            continue;
        if (item1 == nullptr || item2 == nullptr)
            return false;
        if (!item1->Equals(*item2))
            return false;
    }

    return true;
}

void JArray::Add(std::shared_ptr<JToken> item)
{
    items_.push_back(item);
}

void JArray::RemoveAt(int index)
{
    if (index < 0 || static_cast<size_t>(index) >= items_.size())
        throw std::out_of_range("Array index out of range");
    items_.erase(items_.begin() + index);
}

void JArray::Clear()
{
    items_.clear();
}

size_t JArray::Count() const
{
    return items_.size();
}

bool JArray::IsEmpty() const
{
    return items_.empty();
}

const JArray::Items& JArray::GetItems() const
{
    return items_;
}

JArray::Items& JArray::GetItems()
{
    return items_;
}

JArray::Items::iterator JArray::begin()
{
    return items_.begin();
}

JArray::Items::iterator JArray::end()
{
    return items_.end();
}

JArray::Items::const_iterator JArray::begin() const
{
    return items_.begin();
}

JArray::Items::const_iterator JArray::end() const
{
    return items_.end();
}

JArray::Items::const_iterator JArray::cbegin() const
{
    return items_.cbegin();
}

JArray::Items::const_iterator JArray::cend() const
{
    return items_.cend();
}

void JArray::WriteJson(std::string& output, bool indented, int indent_level) const
{
    output += "[";

    if (indented && !items_.empty())
    {
        output += "\n";
    }

    for (size_t i = 0; i < items_.size(); ++i)
    {
        if (indented)
        {
            AddIndentation(output, indent_level + 1);
        }

        const auto& item = items_[i];
        if (item)
        {
            item->WriteJson(output, indented, indent_level + 1);
        }
        else
        {
            output += "null";
        }

        if (i < items_.size() - 1)
        {
            output += ",";
        }

        if (indented)
        {
            output += "\n";
        }
    }

    if (indented && !items_.empty())
    {
        AddIndentation(output, indent_level);
    }

    output += "]";
}
}  // namespace neo::json
