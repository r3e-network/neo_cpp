#include <neo/ledger/witness_rule.h>

#include <stdexcept>

namespace neo::ledger
{

// WitnessRule implementation
WitnessRule::WitnessRule() : action_(WitnessRuleAction::Deny), condition_(nullptr) {}

WitnessRule::WitnessRule(WitnessRuleAction action, std::shared_ptr<WitnessCondition> condition)
    : action_(action), condition_(condition)
{
    if (!condition_)
    {
        throw std::invalid_argument("Condition cannot be null");
    }
}

void WitnessRule::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(action_));
    if (condition_)
    {
        // Write condition type
        writer.Write(static_cast<uint8_t>(condition_->GetType()));
        condition_->Serialize(writer);
    }
    else
    {
        // Write Boolean type with false value as default
        writer.Write(static_cast<uint8_t>(WitnessCondition::Type::Boolean));
        writer.Write(static_cast<uint8_t>(0));
    }
}

void WitnessRule::Deserialize(io::BinaryReader& reader)
{
    action_ = static_cast<WitnessRuleAction>(reader.ReadUInt8());
    if (action_ != WitnessRuleAction::Allow && action_ != WitnessRuleAction::Deny)
    {
        throw std::runtime_error("Invalid witness rule action");
    }

    // Deserialize condition
    condition_ = WitnessCondition::DeserializeFrom(reader, WitnessCondition::MaxNestingDepth);
}

void WitnessRule::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();

    writer.WritePropertyName("action");
    writer.WriteString(action_ == WitnessRuleAction::Allow ? "Allow" : "Deny");

    if (condition_)
    {
        writer.WritePropertyName("condition");
        condition_->SerializeJson(writer);
    }

    writer.WriteEndObject();
}

void WitnessRule::DeserializeJson(const io::JsonReader& reader)
{
    // Simplified JSON deserialization
    // In production, this would need full JSON parsing
    action_ = WitnessRuleAction::Deny;
    condition_ = nullptr;
}

bool WitnessRule::operator==(const WitnessRule& other) const
{
    if (action_ != other.action_) return false;

    if (condition_ && other.condition_)
    {
        // Compare condition types
        return condition_->GetType() == other.condition_->GetType();
    }

    return condition_ == other.condition_;
}

bool WitnessRule::operator!=(const WitnessRule& other) const { return !(*this == other); }

// Basic WitnessCondition factory method
std::shared_ptr<WitnessCondition> WitnessCondition::DeserializeFrom(io::BinaryReader& reader, uint8_t maxDepth)
{
    if (maxDepth == 0)
    {
        throw std::runtime_error("Max nesting depth exceeded");
    }

    auto type = static_cast<Type>(reader.ReadUInt8());

    // For now, return nullptr - in production this would create specific condition types
    // based on the type discriminator
    return nullptr;
}

}  // namespace neo::ledger