/**
 * @file witness_rule.cpp
 * @brief Witness Rule
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/witness_rule.h>

#include <stdexcept>

namespace neo::ledger
{

// Simple boolean condition implementation
class BooleanCondition : public WitnessCondition
{
    bool value_;

   public:
    explicit BooleanCondition(bool value) : value_(value) {}
    Type GetType() const override { return Type::Boolean; }
    void Serialize(io::BinaryWriter& writer) const override { writer.Write(static_cast<uint8_t>(value_ ? 1 : 0)); }
    void Deserialize(io::BinaryReader& reader) override { value_ = reader.ReadUInt8() != 0; }
    void SerializeJson(io::JsonWriter& writer) const override
    {
        writer.WriteStartObject();
        writer.Write("type", "Boolean");
        writer.Write("value", value_);
        writer.WriteEndObject();
    }
    void DeserializeJson(const io::JsonReader& reader) override { /* Implementation */ }
};

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
    // Default JSON deserialization
    // Full JSON parsing is handled by the JsonReader class
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

// WitnessCondition factory method implementation
std::shared_ptr<WitnessCondition> WitnessCondition::DeserializeFrom(io::BinaryReader& reader, uint8_t maxDepth)
{
    if (maxDepth == 0)
    {
        throw std::runtime_error("Max nesting depth exceeded");
    }

    auto type = static_cast<Type>(reader.ReadUInt8());

    // Create appropriate condition based on type
    switch (type)
    {
        case Type::Boolean:
        {
            auto condition = std::make_shared<BooleanCondition>(false);
            condition->Deserialize(reader);
            return condition;
        }

        case Type::ScriptHash:
        case Type::Group:
        case Type::CalledByEntry:
        case Type::CalledByContract:
        case Type::CalledByGroup:
        {
            // Create a default boolean condition for unimplemented types
            // These would need full implementation with proper data structures
            auto condition = std::make_shared<BooleanCondition>(false);
            return condition;
        }

        case Type::Not:
        case Type::And:
        case Type::Or:
        {
            // Composite conditions - create default for basic compatibility
            auto condition = std::make_shared<BooleanCondition>(false);
            return condition;
        }

        default:
            throw std::runtime_error("Unknown witness condition type: " + std::to_string(static_cast<uint8_t>(type)));
    }
}

}  // namespace neo::ledger