#include <gtest/gtest.h>

#include <neo/io/byte_vector.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/witness_rule.h>
#include <neo/network/p2p/payloads/conflicts.h>
#include <neo/network/p2p/payloads/high_priority.h>
#include <neo/network/p2p/payloads/not_valid_before.h>
#include <neo/network/p2p/payloads/oracle_response.h>

#include <nlohmann/json.hpp>

#include <memory>
#include <utility>

namespace
{
template <typename T>
std::pair<T, nlohmann::json> RoundTripJson(const T& original)
{
    nlohmann::json json = nlohmann::json::object();
    neo::io::JsonWriter writer(json);
    original.SerializeJson(writer);

    T reconstructed;
    neo::io::JsonReader reader(json);
    reconstructed.DeserializeJson(reader);

    return {reconstructed, json};
}
}  // namespace

using namespace neo;
using namespace neo::io;
using namespace neo::ledger;
using namespace neo::network::p2p::payloads;

TEST(TransactionAttributeJsonTest, ConflictsRoundTrip)
{
    auto hash = UInt256::Parse("0xbf9eb1a7fcfad8cf4f441f3a9b056c9ad41f3c69bbaef85a4d1297a8b0b8f8f1");
    Conflicts conflicts(hash);

    auto [parsed, originalJson] = RoundTripJson(conflicts);

    EXPECT_EQ(parsed.GetType(), conflicts.GetType());
    EXPECT_EQ(parsed.GetHash(), hash);

    nlohmann::json parsedJson = nlohmann::json::object();
    io::JsonWriter parsedWriter(parsedJson);
    parsed.SerializeJson(parsedWriter);
    EXPECT_EQ(parsedJson, originalJson);
}

TEST(TransactionAttributeJsonTest, OracleResponseRoundTrip)
{
    OracleResponse response;
    response.SetId(1234);
    response.SetCode(OracleResponseCode::Success);
    response.SetResult(io::ByteVector{0x01, 0x02, 0x03});

    auto [parsed, originalJson] = RoundTripJson(response);

    EXPECT_EQ(parsed.GetId(), response.GetId());
    EXPECT_EQ(parsed.GetCode(), response.GetCode());
    EXPECT_EQ(parsed.GetResult(), response.GetResult());

    nlohmann::json parsedJson = nlohmann::json::object();
    io::JsonWriter parsedWriter(parsedJson);
    parsed.SerializeJson(parsedWriter);
    EXPECT_EQ(parsedJson, originalJson);
}

TEST(TransactionAttributeJsonTest, NotValidBeforeRoundTrip)
{
    NotValidBefore attribute(42);

    auto [parsed, originalJson] = RoundTripJson(attribute);

    EXPECT_EQ(parsed.GetType(), attribute.GetType());
    EXPECT_EQ(parsed.GetHeight(), attribute.GetHeight());

    nlohmann::json parsedJson = nlohmann::json::object();
    io::JsonWriter parsedWriter(parsedJson);
    parsed.SerializeJson(parsedWriter);
    EXPECT_EQ(parsedJson, originalJson);
}

TEST(TransactionAttributeJsonTest, HighPriorityRoundTrip)
{
    HighPriority attribute;

    auto [parsed, originalJson] = RoundTripJson(attribute);

    EXPECT_EQ(parsed.GetType(), attribute.GetType());

    nlohmann::json parsedJson = nlohmann::json::object();
    io::JsonWriter parsedWriter(parsedJson);
    parsed.SerializeJson(parsedWriter);
    EXPECT_EQ(parsedJson, originalJson);
}

TEST(WitnessRuleJsonTest, AllowCalledByEntryRoundTrip)
{
    auto condition = std::make_shared<CalledByEntryCondition>();
    WitnessRule rule(WitnessRuleAction::Allow, condition);

    auto [parsed, originalJson] = RoundTripJson(rule);

    EXPECT_EQ(parsed.GetAction(), rule.GetAction());
    auto parsedCondition = std::dynamic_pointer_cast<CalledByEntryCondition>(parsed.GetCondition());
    ASSERT_NE(parsedCondition, nullptr);

    nlohmann::json parsedJson = nlohmann::json::object();
    io::JsonWriter parsedWriter(parsedJson);
    parsed.SerializeJson(parsedWriter);
    EXPECT_EQ(parsedJson, originalJson);
}

TEST(WitnessRuleJsonTest, DenyOrConditionRoundTrip)
{
    auto trueCondition = std::make_shared<BooleanCondition>(true);
    auto falseCondition = std::make_shared<BooleanCondition>(false);

    auto orCondition = std::make_shared<OrCondition>();
    orCondition->SetConditions({trueCondition, falseCondition});

    WitnessRule rule(WitnessRuleAction::Deny, orCondition);

    auto [parsed, originalJson] = RoundTripJson(rule);

    EXPECT_EQ(parsed.GetAction(), rule.GetAction());
    auto parsedCondition = std::dynamic_pointer_cast<OrCondition>(parsed.GetCondition());
    ASSERT_NE(parsedCondition, nullptr);

    nlohmann::json parsedJson = nlohmann::json::object();
    io::JsonWriter parsedWriter(parsedJson);
    parsed.SerializeJson(parsedWriter);
    EXPECT_EQ(parsedJson, originalJson);

    auto conditionJson = parsedJson["condition"];
    ASSERT_TRUE(conditionJson.contains("expressions"));
    const auto& expressions = conditionJson["expressions"];
    ASSERT_EQ(expressions.size(), 2);
    EXPECT_EQ(expressions[0]["type"], "Boolean");
    EXPECT_EQ(expressions[0]["expression"], true);
    EXPECT_EQ(expressions[1]["type"], "Boolean");
    EXPECT_EQ(expressions[1]["expression"], false);
}
