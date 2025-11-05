#include <gtest/gtest.h>

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <nlohmann/json.hpp>
#include <neo/cryptography/ecc/keypair.h>
#include <neo/io/byte_vector.h>
#include <neo/rpc/rpc_client.h>

#include <memory>
#include <string>

namespace
{
using neo::cryptography::ecc::KeyPair;

struct RpcStack
{
    std::string type;
    nlohmann::json value;

    nlohmann::json ToJson() const
    {
        nlohmann::json json;
        json["type"] = type;
        json["value"] = value;
        return json;
    }

    static RpcStack FromJson(const nlohmann::json& json)
    {
        RpcStack stack;
        stack.type = json.value("type", std::string());
        stack.value = json.contains("value") ? json.at("value") : nlohmann::json();
        return stack;
    }
};

std::unique_ptr<KeyPair> GetKeyPairFromString(const std::string& key)
{
    if (key.empty())
    {
        throw std::invalid_argument("key");
    }

    // Try WIF first
    try
    {
        return KeyPair::FromWIF(key);
    }
    catch (const std::exception&)
    {
        // Fall through to hex parsing
    }

    std::string hex = key;
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
    {
        hex = hex.substr(2);
    }

    auto privateKey = neo::io::ByteVector::FromHexString(hex);
    return std::make_unique<KeyPair>(privateKey);
}

boost::multiprecision::cpp_int ToBigInteger(const std::string& amount, uint32_t decimals)
{
    using boost::multiprecision::cpp_dec_float_50;
    using boost::multiprecision::cpp_int;

    if (decimals > 32)
    {
        throw std::invalid_argument("decimal precision too large");
    }

    cpp_dec_float_50 value(amount);
    cpp_dec_float_50 scale = pow(cpp_dec_float_50(10), decimals);
    cpp_dec_float_50 scaled = value * scale;

    cpp_int integerPart = scaled.convert_to<cpp_int>();
    if (scaled != cpp_dec_float_50(integerPart))
    {
        throw std::invalid_argument("Value cannot be represented with requested decimals");
    }

    return integerPart;
}
}  // namespace

TEST(RpcUtilityTests, GetKeyPairSupportsWifAndHex)
{
    const std::string wif = "KyXwTh1hB76RRMquSvnxZrJzQx7h9nQP2PCRL38v6VDb5ip3nf1p";
    auto expected = KeyPair::FromWIF(wif);
    ASSERT_NE(expected, nullptr);

    auto fromWif = GetKeyPairFromString(wif);
    ASSERT_NE(fromWif, nullptr);
    EXPECT_EQ(fromWif->GetPrivateKey().ToHexString(), expected->GetPrivateKey().ToHexString());
    EXPECT_EQ(fromWif->GetPublicKey().ToHex(), expected->GetPublicKey().ToHex());

    const std::string privateKeyHex = expected->GetPrivateKey().ToHexString();
    auto fromHex = GetKeyPairFromString(privateKeyHex);
    ASSERT_NE(fromHex, nullptr);
    EXPECT_EQ(fromHex->GetPrivateKey().ToHexString(), privateKeyHex);

    auto fromHexWithPrefix = GetKeyPairFromString("0x" + privateKeyHex);
    ASSERT_NE(fromHexWithPrefix, nullptr);
    EXPECT_EQ(fromHexWithPrefix->GetPrivateKey().ToHexString(), privateKeyHex);

    EXPECT_THROW(GetKeyPairFromString(""), std::invalid_argument);
    EXPECT_THROW(GetKeyPairFromString("00"), std::invalid_argument);
}

TEST(RpcUtilityTests, ToBigIntegerMatchesCSharpBehaviour)
{
    auto result = ToBigInteger("1.23456789", 9);
    EXPECT_EQ(result.str(), "1234567890");

    auto scaled = ToBigInteger("1.23456789", 18);
    EXPECT_EQ(scaled.str(), "1234567890000000000");

    EXPECT_THROW(ToBigInteger("1.23456789", 4), std::invalid_argument);
}

TEST(RpcUtilityTests, RpcStackRoundTrip)
{
    RpcStack stack;
    stack.type = "Boolean";
    stack.value = true;

    auto json = stack.ToJson();
    EXPECT_TRUE(json.is_object());
    EXPECT_EQ(json.at("type"), stack.type);
    EXPECT_EQ(json.at("value"), stack.value);

    auto parsed = RpcStack::FromJson(json);
    EXPECT_EQ(parsed.type, stack.type);
    EXPECT_EQ(parsed.value, stack.value);
}

TEST(RpcUtilityTests, RpcClientConstructorByUrlDisposes)
{
    EXPECT_NO_THROW({ neo::rpc::RpcClient client("http://localhost:10332"); });
}

TEST(RpcUtilityTests, RpcClientConstructorWithBasicAuth)
{
    EXPECT_NO_THROW({ neo::rpc::RpcClient client("http://localhost:10332", "user", "pass"); });
}
