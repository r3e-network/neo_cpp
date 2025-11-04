#include <neo/ledger/witness_rule.h>

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/contract_management.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

namespace neo::ledger
{
namespace
{
constexpr uint8_t kMaxConditions = WitnessCondition::MaxSubitems;

const char* ActionToString(WitnessRuleAction action)
{
    switch (action)
    {
        case WitnessRuleAction::Allow:
            return "Allow";
        case WitnessRuleAction::Deny:
            return "Deny";
    }
    throw std::runtime_error("Invalid witness rule action");
}

WitnessRuleAction ActionFromString(const std::string& value)
{
    if (value == "Allow") return WitnessRuleAction::Allow;
    if (value == "Deny") return WitnessRuleAction::Deny;
    throw std::runtime_error("Invalid witness rule action string");
}

const char* ConditionTypeToString(WitnessCondition::Type type)
{
    switch (type)
    {
        case WitnessCondition::Type::Boolean:
            return "Boolean";
        case WitnessCondition::Type::Not:
            return "Not";
        case WitnessCondition::Type::And:
            return "And";
        case WitnessCondition::Type::Or:
            return "Or";
        case WitnessCondition::Type::ScriptHash:
            return "ScriptHash";
        case WitnessCondition::Type::Group:
            return "Group";
        case WitnessCondition::Type::CalledByEntry:
            return "CalledByEntry";
        case WitnessCondition::Type::CalledByContract:
            return "CalledByContract";
        case WitnessCondition::Type::CalledByGroup:
            return "CalledByGroup";
    }
    throw std::runtime_error("Unknown witness condition type");
}

WitnessCondition::Type ConditionTypeFromString(const std::string& value)
{
    if (value == "Boolean") return WitnessCondition::Type::Boolean;
    if (value == "Not") return WitnessCondition::Type::Not;
    if (value == "And") return WitnessCondition::Type::And;
    if (value == "Or") return WitnessCondition::Type::Or;
    if (value == "ScriptHash") return WitnessCondition::Type::ScriptHash;
    if (value == "Group") return WitnessCondition::Type::Group;
    if (value == "CalledByEntry") return WitnessCondition::Type::CalledByEntry;
    if (value == "CalledByContract") return WitnessCondition::Type::CalledByContract;
    if (value == "CalledByGroup") return WitnessCondition::Type::CalledByGroup;
    throw std::runtime_error("Unknown witness condition type string");
}

std::shared_ptr<WitnessCondition> CreateCondition(WitnessCondition::Type type)
{
    switch (type)
    {
        case WitnessCondition::Type::Boolean:
            return std::make_shared<BooleanCondition>();
        case WitnessCondition::Type::Not:
            return std::make_shared<NotCondition>();
        case WitnessCondition::Type::And:
            return std::make_shared<AndCondition>();
        case WitnessCondition::Type::Or:
            return std::make_shared<OrCondition>();
        case WitnessCondition::Type::ScriptHash:
            return std::make_shared<ScriptHashCondition>();
        case WitnessCondition::Type::Group:
            return std::make_shared<GroupCondition>();
        case WitnessCondition::Type::CalledByEntry:
            return std::make_shared<CalledByEntryCondition>();
        case WitnessCondition::Type::CalledByContract:
            return std::make_shared<CalledByContractCondition>();
        case WitnessCondition::Type::CalledByGroup:
            return std::make_shared<CalledByGroupCondition>();
    }
    throw std::runtime_error("Unsupported witness condition type");
}
}  // namespace

// WitnessRule --------------------------------------------------------------
WitnessRule::WitnessRule() = default;

WitnessRule::WitnessRule(WitnessRuleAction action, std::shared_ptr<WitnessCondition> condition)
    : action_(action), condition_(std::move(condition))
{
    if (!condition_)
    {
        throw std::invalid_argument("WitnessRule requires a condition");
    }
}

bool WitnessRule::Matches(const smartcontract::ApplicationEngine& engine) const
{
    if (!condition_) return false;
    return condition_->Match(engine);
}

void WitnessRule::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(action_));
    if (condition_)
    {
        condition_->Serialize(writer);
    }
    else
    {
        BooleanCondition(false).Serialize(writer);
    }
}

void WitnessRule::Deserialize(io::BinaryReader& reader)
{
    action_ = static_cast<WitnessRuleAction>(reader.ReadUInt8());
    if (action_ != WitnessRuleAction::Allow && action_ != WitnessRuleAction::Deny)
    {
        throw std::runtime_error("Invalid witness rule action");
    }
    condition_ = WitnessCondition::DeserializeFrom(reader, WitnessCondition::MaxNestingDepth);
}

void WitnessRule::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.Write("action", std::string(ActionToString(action_)));
    writer.WritePropertyName("condition");
    if (condition_)
    {
        condition_->SerializeJson(writer);
    }
    else
    {
        BooleanCondition(false).SerializeJson(writer);
    }
    writer.WriteEndObject();
}

void WitnessRule::DeserializeJson(const io::JsonReader& reader)
{
    if (!reader.HasKey("action")) throw std::runtime_error("WitnessRule missing action");
    action_ = ActionFromString(reader.ReadString("action"));

    if (!reader.HasKey("condition")) throw std::runtime_error("WitnessRule missing condition");
    auto conditionJson = reader.ReadObject("condition");
    if (!conditionJson.is_object()) throw std::runtime_error("WitnessRule condition must be an object");
    io::JsonReader conditionReader(conditionJson);
    condition_ = WitnessCondition::FromJson(conditionReader, WitnessCondition::MaxNestingDepth);
}

bool WitnessRule::operator==(const WitnessRule& other) const
{
    if (action_ != other.action_) return false;
    if (condition_ == other.condition_) return true;
    if (!condition_ || !other.condition_) return false;
    return condition_->GetType() == other.condition_->GetType();
}

bool WitnessRule::operator!=(const WitnessRule& other) const { return !(*this == other); }

// WitnessCondition base ----------------------------------------------------

void WitnessCondition::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(static_cast<uint8_t>(GetType()));
    SerializeWithoutType(writer);
}

void WitnessCondition::Deserialize(io::BinaryReader& reader)
{
    auto type = static_cast<Type>(reader.ReadUInt8());
    if (type != GetType()) throw std::runtime_error("WitnessCondition type mismatch during deserialize");
    DeserializeWithoutType(reader, MaxNestingDepth);
}

void WitnessCondition::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    writer.Write("type", std::string(ConditionTypeToString(GetType())));
    WriteJsonFields(writer);
    writer.WriteEndObject();
}

void WitnessCondition::DeserializeJson(const io::JsonReader& reader)
{
    if (!reader.HasKey("type")) throw std::runtime_error("WitnessCondition missing type");
    auto type = ConditionTypeFromString(reader.ReadString("type"));
    if (type != GetType()) throw std::runtime_error("WitnessCondition type mismatch during JSON parse");
    ParseJsonInternal(reader, MaxNestingDepth);
}

std::shared_ptr<WitnessCondition> WitnessCondition::DeserializeFrom(io::BinaryReader& reader, uint8_t maxDepth)
{
    if (maxDepth == 0) throw std::runtime_error("WitnessCondition nesting depth exceeded");
    auto type = static_cast<Type>(reader.ReadUInt8());
    auto condition = CreateCondition(type);
    condition->DeserializeWithoutType(reader, maxDepth);
    return condition;
}

std::shared_ptr<WitnessCondition> WitnessCondition::FromJson(const io::JsonReader& reader, uint8_t maxDepth)
{
    if (maxDepth == 0) throw std::runtime_error("WitnessCondition JSON nesting depth exceeded");
    if (!reader.HasKey("type")) throw std::runtime_error("WitnessCondition JSON missing type");
    auto type = ConditionTypeFromString(reader.ReadString("type"));
    auto condition = CreateCondition(type);
    condition->ParseJsonInternal(reader, maxDepth);
    return condition;
}

void WitnessCondition::ParseJsonInternal(const io::JsonReader&, uint8_t) {}

void WitnessCondition::WriteJsonFields(io::JsonWriter&) const {}

void WitnessCondition::SerializeConditionArray(io::BinaryWriter& writer,
                                               const std::vector<std::shared_ptr<WitnessCondition>>& conditions)
{
    if (conditions.size() > kMaxConditions) throw std::runtime_error("Too many witness conditions");
    writer.WriteVarInt(conditions.size());
    for (const auto& condition : conditions)
    {
        if (!condition) throw std::runtime_error("Null witness condition");
        condition->Serialize(writer);
    }
}

std::vector<std::shared_ptr<WitnessCondition>> WitnessCondition::DeserializeConditionArray(io::BinaryReader& reader,
                                                                                           uint8_t maxDepth)
{
    auto count = reader.ReadVarInt();
    if (count > kMaxConditions) throw std::runtime_error("Too many witness conditions");
    std::vector<std::shared_ptr<WitnessCondition>> result;
    result.reserve(static_cast<size_t>(count));
    for (uint64_t i = 0; i < count; ++i)
    {
        auto nextDepth = maxDepth == 0 ? static_cast<uint8_t>(0) : static_cast<uint8_t>(maxDepth - 1);
        result.push_back(DeserializeFrom(reader, nextDepth));
    }
    return result;
}

void WitnessCondition::WriteConditionArray(io::JsonWriter& writer, const std::string& key,
                                           const std::vector<std::shared_ptr<WitnessCondition>>& conditions)
{
    writer.WritePropertyName(key);
    writer.WriteStartArray();
    for (const auto& condition : conditions)
    {
        if (!condition) throw std::runtime_error("Null witness condition");
        condition->SerializeJson(writer);
    }
    writer.WriteEndArray();
}

std::vector<std::shared_ptr<WitnessCondition>> WitnessCondition::ParseConditionArray(const io::JsonReader& reader,
                                                                                      const std::string& key,
                                                                                      uint8_t maxDepth)
{
    if (!reader.HasKey(key)) return {};

    auto array = reader.ReadArray(key);
    if (!array.is_array()) throw std::runtime_error("WitnessCondition JSON array expected");
    if (array.size() > kMaxConditions) throw std::runtime_error("Too many witness conditions in JSON");

    std::vector<std::shared_ptr<WitnessCondition>> result;
    result.reserve(array.size());
    for (const auto& item : array)
    {
        io::JsonReader itemReader(item);
        auto nextDepth = maxDepth == 0 ? static_cast<uint8_t>(0) : static_cast<uint8_t>(maxDepth - 1);
        result.push_back(FromJson(itemReader, nextDepth));
    }
    return result;
}

// BooleanCondition ----------------------------------------------------------
BooleanCondition::BooleanCondition(bool value) : value_(value) {}

bool BooleanCondition::Match(const smartcontract::ApplicationEngine&) const { return value_; }

void BooleanCondition::SerializeWithoutType(io::BinaryWriter& writer) const { writer.Write(static_cast<uint8_t>(value_)); }

void BooleanCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t) { value_ = reader.ReadUInt8() != 0; }

void BooleanCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t)
{
    if (!reader.HasKey("expression")) throw std::runtime_error("BooleanCondition missing expression");
    value_ = reader.ReadBool("expression");
}

void BooleanCondition::WriteJsonFields(io::JsonWriter& writer) const { writer.Write("expression", value_); }

// NotCondition --------------------------------------------------------------
NotCondition::NotCondition() = default;
NotCondition::NotCondition(std::shared_ptr<WitnessCondition> condition) : condition_(std::move(condition)) {}

bool NotCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return condition_ ? !condition_->Match(engine) : true;
}

void NotCondition::SerializeWithoutType(io::BinaryWriter& writer) const
{
    if (!condition_) throw std::runtime_error("NotCondition missing operand");
    condition_->Serialize(writer);
}

void NotCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth)
{
    condition_ = DeserializeFrom(reader, maxDepth == 0 ? static_cast<uint8_t>(0) : static_cast<uint8_t>(maxDepth - 1));
}

void NotCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth)
{
    if (!reader.HasKey("expression")) throw std::runtime_error("NotCondition missing expression");
    auto expression = reader.ReadObject("expression");
    if (!expression.is_object()) throw std::runtime_error("NotCondition expression must be an object");
    io::JsonReader exprReader(expression);
    condition_ = FromJson(exprReader, maxDepth == 0 ? static_cast<uint8_t>(0) : static_cast<uint8_t>(maxDepth - 1));
}

void NotCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    writer.WritePropertyName("expression");
    if (condition_)
    {
        condition_->SerializeJson(writer);
    }
    else
    {
        BooleanCondition(false).SerializeJson(writer);
    }
}

// AndCondition --------------------------------------------------------------
void AndCondition::SetConditions(std::vector<std::shared_ptr<WitnessCondition>> conditions)
{
    if (conditions.size() > kMaxConditions) throw std::runtime_error("Too many And operands");
    conditions_ = std::move(conditions);
}

bool AndCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return std::all_of(conditions_.begin(), conditions_.end(),
                       [&](const std::shared_ptr<WitnessCondition>& cond) { return cond->Match(engine); });
}

void AndCondition::SerializeWithoutType(io::BinaryWriter& writer) const
{
    SerializeConditionArray(writer, conditions_);
}

void AndCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth)
{
    conditions_ = DeserializeConditionArray(reader, maxDepth);
    if (conditions_.empty()) throw std::runtime_error("AndCondition expressions cannot be empty");
}

void AndCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth)
{
    if (!reader.HasKey("expressions")) throw std::runtime_error("AndCondition missing expressions");
    conditions_ = ParseConditionArray(reader, "expressions", maxDepth);
    if (conditions_.empty()) throw std::runtime_error("AndCondition expressions cannot be empty");
}

void AndCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    WriteConditionArray(writer, "expressions", conditions_);
}

// OrCondition ---------------------------------------------------------------
void OrCondition::SetConditions(std::vector<std::shared_ptr<WitnessCondition>> conditions)
{
    if (conditions.size() > kMaxConditions) throw std::runtime_error("Too many Or operands");
    conditions_ = std::move(conditions);
}

bool OrCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return std::any_of(conditions_.begin(), conditions_.end(),
                       [&](const std::shared_ptr<WitnessCondition>& cond) { return cond->Match(engine); });
}

void OrCondition::SerializeWithoutType(io::BinaryWriter& writer) const { SerializeConditionArray(writer, conditions_); }

void OrCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth)
{
    conditions_ = DeserializeConditionArray(reader, maxDepth);
    if (conditions_.empty()) throw std::runtime_error("OrCondition expressions cannot be empty");
}

void OrCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth)
{
    if (!reader.HasKey("expressions")) throw std::runtime_error("OrCondition missing expressions");
    conditions_ = ParseConditionArray(reader, "expressions", maxDepth);
    if (conditions_.empty()) throw std::runtime_error("OrCondition expressions cannot be empty");
}

void OrCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    WriteConditionArray(writer, "expressions", conditions_);
}

// ScriptHashCondition -------------------------------------------------------
ScriptHashCondition::ScriptHashCondition() = default;
ScriptHashCondition::ScriptHashCondition(const io::UInt160& hash) : hash_(hash) {}

bool ScriptHashCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return engine.GetCallingScriptHash() == hash_;
}

void ScriptHashCondition::SerializeWithoutType(io::BinaryWriter& writer) const { hash_.Serialize(writer); }

void ScriptHashCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t)
{
    hash_.Deserialize(reader);
}

void ScriptHashCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t)
{
    if (!reader.HasKey("hash")) throw std::runtime_error("ScriptHashCondition missing hash");
    std::string hashStr = reader.ReadString("hash");
    if (hashStr.rfind("0x", 0) == 0) hashStr.erase(0, 2);
    hash_ = io::UInt160::Parse(hashStr);
}

void ScriptHashCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    writer.WriteString("hash", "0x" + hash_.ToHexString());
}

// GroupCondition ------------------------------------------------------------
GroupCondition::GroupCondition() = default;
GroupCondition::GroupCondition(const cryptography::ecc::ECPoint& group) : group_(group) {}

bool GroupCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return engine.IsContractGroupMember(group_);
}

void GroupCondition::SerializeWithoutType(io::BinaryWriter& writer) const { group_.Serialize(writer); }

void GroupCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t)
{
    group_.Deserialize(reader);
}

void GroupCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t)
{
    if (!reader.HasKey("group")) throw std::runtime_error("GroupCondition missing group");
    auto groupStr = reader.ReadString("group");
    if (groupStr.empty()) throw std::runtime_error("GroupCondition group cannot be empty");
    group_ = cryptography::ecc::ECPoint::FromHex(groupStr);
}

void GroupCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    writer.WriteString("group", group_.ToHex());
}

// CalledByEntryCondition ----------------------------------------------------
bool CalledByEntryCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return engine.IsCalledByEntry();
}

void CalledByEntryCondition::SerializeWithoutType(io::BinaryWriter&) const {}

void CalledByEntryCondition::DeserializeWithoutType(io::BinaryReader&, uint8_t) {}

void CalledByEntryCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t)
{
    if (reader.HasKey("expression") || reader.HasKey("hash") || reader.HasKey("group"))
    {
        throw std::runtime_error("CalledByEntryCondition should not contain additional fields");
    }
}

// CalledByContractCondition -------------------------------------------------
CalledByContractCondition::CalledByContractCondition() = default;
CalledByContractCondition::CalledByContractCondition(const io::UInt160& hash) : hash_(hash) {}

bool CalledByContractCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return engine.GetCallingScriptHash() == hash_;
}

void CalledByContractCondition::SerializeWithoutType(io::BinaryWriter& writer) const { hash_.Serialize(writer); }

void CalledByContractCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t)
{
    hash_.Deserialize(reader);
}

void CalledByContractCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t)
{
    if (!reader.HasKey("hash")) throw std::runtime_error("CalledByContractCondition missing hash");
    std::string hashStr = reader.ReadString("hash");
    if (hashStr.rfind("0x", 0) == 0) hashStr.erase(0, 2);
    hash_ = io::UInt160::Parse(hashStr);
}

void CalledByContractCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    writer.WriteString("hash", "0x" + hash_.ToHexString());
}

// CalledByGroupCondition ----------------------------------------------------
CalledByGroupCondition::CalledByGroupCondition() = default;
CalledByGroupCondition::CalledByGroupCondition(const cryptography::ecc::ECPoint& group) : group_(group) {}

bool CalledByGroupCondition::Match(const smartcontract::ApplicationEngine& engine) const
{
    return engine.IsContractGroupMember(group_);
}

void CalledByGroupCondition::SerializeWithoutType(io::BinaryWriter& writer) const { group_.Serialize(writer); }

void CalledByGroupCondition::DeserializeWithoutType(io::BinaryReader& reader, uint8_t)
{
    group_.Deserialize(reader);
}

void CalledByGroupCondition::ParseJsonInternal(const io::JsonReader& reader, uint8_t)
{
    if (!reader.HasKey("group")) throw std::runtime_error("CalledByGroupCondition missing group");
    auto groupStr = reader.ReadString("group");
    if (groupStr.empty()) throw std::runtime_error("CalledByGroupCondition group cannot be empty");
    group_ = cryptography::ecc::ECPoint::FromHex(groupStr);
}

void CalledByGroupCondition::WriteJsonFields(io::JsonWriter& writer) const
{
    writer.WriteString("group", group_.ToHex());
}

}  // namespace neo::ledger
