/**
 * @file signer.cpp
 * @brief Signer
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/ledger/signer.h>
#include <neo/ledger/witness_rule.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace
{
constexpr int kMaxSubitems = 16;

std::string Trim(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return std::string();
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string WitnessScopeToString(neo::ledger::WitnessScope scopes)
{
    using neo::ledger::WitnessScope;
    if (scopes == WitnessScope::Global) return "Global";
    if (scopes == WitnessScope::None) return "None";

    std::vector<std::string> parts;
    if ((static_cast<uint8_t>(scopes) & static_cast<uint8_t>(WitnessScope::CalledByEntry)) != 0)
        parts.emplace_back("CalledByEntry");
    if ((static_cast<uint8_t>(scopes) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
        parts.emplace_back("CustomContracts");
    if ((static_cast<uint8_t>(scopes) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
        parts.emplace_back("CustomGroups");
    if ((static_cast<uint8_t>(scopes) & static_cast<uint8_t>(WitnessScope::WitnessRules)) != 0)
        parts.emplace_back("WitnessRules");
    if (parts.empty()) return "None";

    std::ostringstream oss;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i != 0) oss << ", ";
        oss << parts[i];
    }
    return oss.str();
}

neo::ledger::WitnessScope ParseWitnessScope(const std::string& input)
{
    using neo::ledger::WitnessScope;
    auto text = Trim(input);
    if (text.empty() || text == "None") return WitnessScope::None;

    // Accept numeric values for backwards compatibility.
    if (std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c); }))
    {
        auto value = static_cast<uint8_t>(std::stoi(text));
        return static_cast<WitnessScope>(value);
    }

    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        token = Trim(token);
        if (!token.empty()) tokens.emplace_back(std::move(token));
    }
    if (tokens.empty()) return WitnessScope::None;
    if (tokens.size() == 1 && tokens[0] == "Global") return WitnessScope::Global;

    WitnessScope result = WitnessScope::None;
    for (const auto& name : tokens)
    {
        if (name == "None")
        {
            continue;
        }
        if (name == "Global")
        {
            throw std::runtime_error("Global scope cannot be combined with other scopes");
        }
        if (name == "CalledByEntry")
        {
            result |= WitnessScope::CalledByEntry;
        }
        else if (name == "CustomContracts")
        {
            result |= WitnessScope::CustomContracts;
        }
        else if (name == "CustomGroups")
        {
            result |= WitnessScope::CustomGroups;
        }
        else if (name == "WitnessRules")
        {
            result |= WitnessScope::WitnessRules;
        }
        else
        {
            throw std::runtime_error("Unknown witness scope: " + name);
        }
    }

    return result;
}

void ValidateScopes(neo::ledger::WitnessScope scopes)
{
    using neo::ledger::WitnessScope;
    constexpr uint8_t allowed =
        static_cast<uint8_t>(WitnessScope::CalledByEntry) | static_cast<uint8_t>(WitnessScope::CustomContracts) |
        static_cast<uint8_t>(WitnessScope::CustomGroups) | static_cast<uint8_t>(WitnessScope::WitnessRules) |
        static_cast<uint8_t>(WitnessScope::Global);

    const uint8_t value = static_cast<uint8_t>(scopes);
    if ((value & ~allowed) != 0)
    {
        throw std::runtime_error("Invalid witness scope bits");
    }
    if ((value & static_cast<uint8_t>(WitnessScope::Global)) != 0 && scopes != WitnessScope::Global)
    {
        throw std::runtime_error("Global scope cannot be combined with other scopes");
    }
}
}  // namespace

namespace neo::ledger
{
Signer::Signer() : account_(), scopes_(WitnessScope::None) {}

Signer::Signer(const io::UInt160& account, WitnessScope scopes) : account_(account), scopes_(scopes) {}

const io::UInt160& Signer::GetAccount() const { return account_; }

void Signer::SetAccount(const io::UInt160& account) { account_ = account; }

WitnessScope Signer::GetScopes() const { return scopes_; }

void Signer::SetScopes(WitnessScope scopes)
{
    ValidateScopes(scopes);
    scopes_ = scopes;
}

const std::vector<io::UInt160>& Signer::GetAllowedContracts() const { return allowedContracts_; }

void Signer::SetAllowedContracts(const std::vector<io::UInt160>& allowedContracts)
{
    allowedContracts_ = allowedContracts;
}

const std::vector<cryptography::ecc::ECPoint>& Signer::GetAllowedGroups() const { return allowedGroups_; }

void Signer::SetAllowedGroups(const std::vector<cryptography::ecc::ECPoint>& allowedGroups)
{
    allowedGroups_ = allowedGroups;
}

const std::vector<WitnessRule>& Signer::GetRules() const { return rules_; }

void Signer::SetRules(const std::vector<WitnessRule>& rules) { rules_ = rules; }

std::vector<WitnessRule> Signer::GetAllRules() const
{
    std::vector<WitnessRule> rules;

    auto add_boolean_rule = [&](bool value)
    {
        auto condition = std::make_shared<BooleanCondition>(value);
        rules.emplace_back(WitnessRuleAction::Allow, condition);
    };

    if (scopes_ == WitnessScope::Global)
    {
        add_boolean_rule(true);
        return rules;
    }

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CalledByEntry)) != 0)
    {
        rules.emplace_back(WitnessRuleAction::Allow, std::make_shared<CalledByEntryCondition>());
    }

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
    {
        for (const auto& contract : allowedContracts_)
        {
            rules.emplace_back(WitnessRuleAction::Allow, std::make_shared<ScriptHashCondition>(contract));
        }
    }

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
    {
        for (const auto& group : allowedGroups_)
        {
            rules.emplace_back(WitnessRuleAction::Allow, std::make_shared<GroupCondition>(group));
        }
    }

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::WitnessRules)) != 0)
    {
        rules.insert(rules.end(), rules_.begin(), rules_.end());
    }

    return rules;
}

void Signer::Serialize(io::BinaryWriter& writer) const
{
    // Serialize account
    account_.Serialize(writer);

    // Serialize scopes
    writer.Write(static_cast<uint8_t>(scopes_));

    // Serialize allowed contracts if CustomContracts scope is set
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
    {
        if (allowedContracts_.size() > static_cast<size_t>(kMaxSubitems))
            throw std::runtime_error("allowedContracts exceeds maximum");
        writer.WriteVarInt(allowedContracts_.size());
        for (const auto& contract : allowedContracts_)
        {
            contract.Serialize(writer);
        }
    }

    // Serialize allowed groups if CustomGroups scope is set
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
    {
        if (allowedGroups_.size() > static_cast<size_t>(kMaxSubitems))
            throw std::runtime_error("allowedGroups exceeds maximum");
        writer.WriteVarInt(allowedGroups_.size());
        for (const auto& group : allowedGroups_)
        {
            auto groupBytes = group.ToArray();
            writer.WriteBytes(groupBytes.Data(), groupBytes.Size());
        }
    }

    // Serialize rules if WitnessRules scope is set
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::WitnessRules)) != 0)
    {
        if (rules_.size() > static_cast<size_t>(kMaxSubitems))
            throw std::runtime_error("rules exceeds maximum");
        writer.WriteVarInt(rules_.size());
        for (const auto& rule : rules_)
        {
            rule.Serialize(writer);
        }
    }
}

void Signer::Deserialize(io::BinaryReader& reader)
{
    // Deserialize account
    account_.Deserialize(reader);

    // Deserialize scopes
    scopes_ = static_cast<WitnessScope>(reader.ReadUInt8());
    ValidateScopes(scopes_);

    // Deserialize allowed contracts if CustomContracts scope is set
    allowedContracts_.clear();
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
    {
        int64_t contractCount = reader.ReadVarInt();
        if (contractCount < 0 || contractCount > 16) throw std::out_of_range("Invalid allowed contracts count");

        allowedContracts_.reserve(static_cast<size_t>(contractCount));
        for (int64_t i = 0; i < contractCount; i++)
        {
            io::UInt160 contract;
            contract.Deserialize(reader);
            allowedContracts_.push_back(contract);
        }
    }

    // Deserialize allowed groups if CustomGroups scope is set
    allowedGroups_.clear();
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
    {
        int64_t groupCount = reader.ReadVarInt();
        if (groupCount < 0 || groupCount > 16) throw std::out_of_range("Invalid allowed groups count");

        allowedGroups_.reserve(static_cast<size_t>(groupCount));
        for (int64_t i = 0; i < groupCount; i++)
        {
            auto groupBytes = reader.ReadBytes(33);  // ECPoint is 33 bytes compressed
            auto group =
                cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(groupBytes.Data(), groupBytes.Size()), "secp256r1");
            allowedGroups_.push_back(group);
        }
    }

    // Deserialize rules if WitnessRules scope is set
    rules_.clear();
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::WitnessRules)) != 0)
    {
        int64_t ruleCount = reader.ReadVarInt();
        if (ruleCount < 0 || ruleCount > kMaxSubitems) throw std::out_of_range("Invalid rules count");

        rules_.reserve(static_cast<size_t>(ruleCount));
        for (int64_t i = 0; i < ruleCount; i++)
        {
            WitnessRule rule;
            rule.Deserialize(reader);
            rules_.push_back(rule);
        }
    }
}

void Signer::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();

    writer.WritePropertyName("account");
    writer.WriteString("0x" + account_.ToHexString());

    writer.WritePropertyName("scopes");
    writer.WriteString(WitnessScopeToString(scopes_));

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
    {
        writer.WritePropertyName("allowedcontracts");
        writer.WriteStartArray();
        for (const auto& contract : allowedContracts_)
        {
            writer.WriteString("0x" + contract.ToHexString());
        }
        writer.WriteEndArray();
    }

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
    {
        writer.WritePropertyName("allowedgroups");
        writer.WriteStartArray();
        for (const auto& group : allowedGroups_)
        {
            writer.WriteString(group.ToHex());
        }
        writer.WriteEndArray();
    }

    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::WitnessRules)) != 0)
    {
        writer.WritePropertyName("rules");
        writer.WriteStartArray();
        for (const auto& rule : rules_)
        {
            rule.SerializeJson(writer);
        }
        writer.WriteEndArray();
    }

    writer.WriteEndObject();
}

void Signer::DeserializeJson(const io::JsonReader& reader)
{
    // Read account (remove "0x" prefix if present)
    std::string accountStr = reader.ReadString("account");
    if (accountStr.length() >= 2 && accountStr.substr(0, 2) == "0x")
    {
        accountStr = accountStr.substr(2);
    }
    account_ = io::UInt160::Parse(accountStr);

    // Read scopes (string preferred, fallback to integer)
    WitnessScope parsedScopes = WitnessScope::None;
    if (reader.HasKey("scopes"))
    {
        std::string scopeStr = reader.ReadString("scopes");
        if (!scopeStr.empty())
        {
            parsedScopes = ParseWitnessScope(scopeStr);
        }
        else
        {
            parsedScopes = static_cast<WitnessScope>(reader.ReadInt32("scopes"));
        }
    }
    ValidateScopes(parsedScopes);
    scopes_ = parsedScopes;

    // Clear existing arrays
    allowedContracts_.clear();
    allowedGroups_.clear();
    rules_.clear();

    // Read allowed contracts if CustomContracts scope is set
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomContracts)) != 0)
    {
        if (!reader.HasKey("allowedcontracts")) throw std::runtime_error("Signer missing allowedcontracts");
        auto contractsArray = reader.ReadArray("allowedcontracts");
        if (!contractsArray.is_array()) throw std::runtime_error("allowedcontracts must be an array");
        if (contractsArray.size() > kMaxSubitems) throw std::runtime_error("allowedcontracts exceeds limit");
        allowedContracts_.reserve(contractsArray.size());
        for (const auto& contractJson : contractsArray)
        {
            if (!contractJson.is_string()) throw std::runtime_error("allowedcontracts entries must be strings");
            std::string contractStr = contractJson.get<std::string>();
            if (contractStr.length() >= 2 && contractStr.substr(0, 2) == "0x")
            {
                contractStr = contractStr.substr(2);
            }
            allowedContracts_.push_back(io::UInt160::Parse(contractStr));
        }
    }

    // Read allowed groups if CustomGroups scope is set
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::CustomGroups)) != 0)
    {
        if (!reader.HasKey("allowedgroups")) throw std::runtime_error("Signer missing allowedgroups");
        auto groupsArray = reader.ReadArray("allowedgroups");
        if (!groupsArray.is_array()) throw std::runtime_error("allowedgroups must be an array");
        if (groupsArray.size() > kMaxSubitems) throw std::runtime_error("allowedgroups exceeds limit");
        allowedGroups_.reserve(groupsArray.size());
        for (const auto& groupJson : groupsArray)
        {
            if (!groupJson.is_string()) throw std::runtime_error("allowedgroups entries must be strings");
            std::string groupStr = groupJson.get<std::string>();
            auto groupBytes = io::ByteVector::ParseHex(groupStr);
            auto group =
                cryptography::ecc::ECPoint::FromBytes(io::ByteSpan(groupBytes.Data(), groupBytes.Size()), "secp256r1");
            allowedGroups_.push_back(group);
        }
    }
    if ((static_cast<uint8_t>(scopes_) & static_cast<uint8_t>(WitnessScope::WitnessRules)) != 0)
    {
        if (!reader.HasKey("rules")) throw std::runtime_error("Signer missing rules");
        auto rulesArray = reader.ReadArray("rules");
        if (!rulesArray.is_array()) throw std::runtime_error("rules must be an array");
        if (rulesArray.size() > kMaxSubitems) throw std::runtime_error("rules exceeds limit");
        rules_.reserve(rulesArray.size());
        for (const auto& ruleJson : rulesArray)
        {
            if (!ruleJson.is_object()) throw std::runtime_error("rules entries must be objects");
            WitnessRule rule;
            io::JsonReader ruleReader(ruleJson);
            rule.DeserializeJson(ruleReader);
            rules_.push_back(rule);
        }
    }
}

bool Signer::operator==(const Signer& other) const
{
    return account_ == other.account_ && scopes_ == other.scopes_ && allowedContracts_ == other.allowedContracts_ &&
           allowedGroups_ == other.allowedGroups_ && rules_ == other.rules_;
}

bool Signer::operator!=(const Signer& other) const { return !(*this == other); }
}  // namespace neo::ledger
