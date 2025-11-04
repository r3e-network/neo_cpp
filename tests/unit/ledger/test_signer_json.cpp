#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <memory>
#include <vector>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/ledger/signer.h>
#include <neo/ledger/witness_rule.h>

using namespace neo::ledger;
using namespace neo::io;

TEST(SignerJsonTest, SerializeMatchesReferenceFormat)
{
    Signer signer;
    signer.SetAccount(UInt160::Parse("0123456789ABCDEF0123456789ABCDEF01234567"));
    signer.SetScopes(WitnessScope::CalledByEntry | WitnessScope::CustomContracts | WitnessScope::WitnessRules);
    signer.SetAllowedContracts(
        std::vector<UInt160>{UInt160::Parse("89ABCDEF0123456789ABCDEF0123456789ABCDEF")});

    WitnessRule rule(WitnessRuleAction::Allow, std::make_shared<BooleanCondition>(true));
    signer.SetRules({rule});

    nlohmann::json json = nlohmann::json::object();
    JsonWriter writer(json);
    signer.SerializeJson(writer);

    EXPECT_EQ(json.at("account"), "0x0123456789abcdef0123456789abcdef01234567");
    EXPECT_EQ(json.at("scopes"), "CalledByEntry, CustomContracts, WitnessRules");

    ASSERT_TRUE(json.contains("allowedcontracts"));
    ASSERT_TRUE(json["allowedcontracts"].is_array());
    ASSERT_EQ(json["allowedcontracts"].size(), 1);
    EXPECT_EQ(json["allowedcontracts"][0], "0x89abcdef0123456789abcdef0123456789abcdef");

    ASSERT_TRUE(json.contains("rules"));
    ASSERT_TRUE(json["rules"].is_array());
    ASSERT_EQ(json["rules"].size(), 1);
    const auto& ruleJson = json["rules"][0];
    EXPECT_EQ(ruleJson.at("action"), "Allow");
    EXPECT_EQ(ruleJson.at("condition").at("type"), "Boolean");
    EXPECT_TRUE(ruleJson.at("condition").at("expression").get<bool>());
}

TEST(SignerJsonTest, DeserializeParsesStringScopesAndRules)
{
    nlohmann::json json = nlohmann::json::object();
    json["account"] = "0xFEDCBA9876543210FEDCBA9876543210FEDCBA98";
    json["scopes"] = "CalledByEntry, CustomGroups, WitnessRules";
    const std::string groupHex = "03b209fd4f53a077d5fae72a9a0f5ac59b73fa4cbb4904caaed2dd49f4e0d8a110";
    json["allowedgroups"] = nlohmann::json::array({groupHex});
    json["rules"] = nlohmann::json::array(
        {nlohmann::json{{"action", "Allow"}, {"condition", nlohmann::json{{"type", "CalledByEntry"}}}}});

    Signer signer;
    JsonReader reader(json);
    signer.DeserializeJson(reader);

    auto to_lower = [](std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    };

    EXPECT_EQ(to_lower(signer.GetAccount().ToHexString()), "fedcba9876543210fedcba9876543210fedcba98");
    EXPECT_EQ(signer.GetScopes(),
              WitnessScope::CalledByEntry | WitnessScope::CustomGroups | WitnessScope::WitnessRules);

    const auto& groups = signer.GetAllowedGroups();
    ASSERT_EQ(groups.size(), 1);
    EXPECT_EQ(to_lower(groups[0].ToHex()), to_lower(groupHex));

    const auto& rules = signer.GetRules();
    ASSERT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].GetAction(), WitnessRuleAction::Allow);
    ASSERT_TRUE(rules[0].GetCondition());
    EXPECT_EQ(rules[0].GetCondition()->GetType(), WitnessCondition::Type::CalledByEntry);
}
