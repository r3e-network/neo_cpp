#include <neo/smartcontract/application_engine.h>

#include <gtest/gtest.h>
#include <neo/cryptography/hash.h>
#include <neo/io/byte_span.h>

#include <cstring>

namespace neo::smartcontract
{
class ApplicationEngineTestAccessor
{
   public:
    static std::vector<cryptography::ecc::ECPoint> GetCommittee(ApplicationEngine& engine) { return engine.GetCommittee(); }

    static io::ByteVector CreateCommitteeMultiSigScript(ApplicationEngine& engine,
                                                        const std::vector<cryptography::ecc::ECPoint>& committee)
    {
        return engine.CreateCommitteeMultiSigScript(committee);
    }

    static void PushScriptHash(ApplicationEngine& engine, const io::UInt160& hash) { engine.scriptHashes_.push_back(hash); }

    static void SetStandbyCommittee(ApplicationEngine& engine,
                                    const std::vector<cryptography::ecc::ECPoint>& committee)
    {
        engine.protocolSettings_.SetStandbyCommittee(committee);
    }

    static bool IsCommitteeHash(const ApplicationEngine& engine, const io::UInt256& hash)
    {
        return engine.IsCommitteeHash(hash);
    }

    static bool VerifyCommitteeConsensus(const ApplicationEngine& engine, const io::UInt256& hash)
    {
        return engine.VerifyCommitteeConsensus(hash);
    }

    static bool VerifyMultiSignatureHash(const ApplicationEngine& engine, const io::UInt256& hash)
    {
        return engine.VerifyMultiSignatureHash(hash);
    }
};
}  // namespace neo::smartcontract

namespace
{
neo::io::UInt256 BuildConsensusHashFromScriptHash(const neo::io::UInt160& scriptHash)
{
    neo::io::UInt256 value = neo::io::UInt256::Zero();
    std::memcpy(value.Data(), scriptHash.Data(), neo::io::UInt160::Size);
    return value;
}
}  // namespace

namespace neo::smartcontract
{
TEST(ApplicationEngineCommittee, DetectsStandbyCommitteeHash)
{
    ApplicationEngine engine(TriggerType::Application, nullptr, nullptr, nullptr, ApplicationEngine::TestModeGas);

    const std::vector<std::string> committeeHex = {
        "03b209fd4fbe4a85d51fa67819c59ab4a8b3443f15c289086a9f3df5a3322b3f90",
        "02bca21b6a2ac0f1db0e3fa029bdb83afd05e2ad1e1c167539ddc2418630af6f79",
        "0207350d87ff9f0e2dfb0f0b547044583b1f99bade25d67f1055d1d217fe7f7554",
        "03ab2f4f40f4f06bdbd293c9c530f5dbe9a359d8a20b19be3cfa4d8e436a6fd9de",
        "0310c9ffb73e2dc89c1f0f40d0e1c6cfb3d80f0a8d4f6d26d7a64c4fbc94c95f7b",
        "03c54d5cd05c437d2b6b1c01cf9f1831ba4f843a95e6fcd508a20ef2ff92d55b1b",
        "03986ce5b5eb1b3e8da1aba9205a721e8c5d1cd6d895c4b373f1b9c2a43959fd0d"};

    std::vector<cryptography::ecc::ECPoint> committee;
    committee.reserve(committeeHex.size());
    for (const auto& hex : committeeHex)
    {
        committee.emplace_back(cryptography::ecc::ECPoint::FromHex(hex));
    }

    ApplicationEngineTestAccessor::SetStandbyCommittee(engine, committee);

    auto resolvedCommittee = ApplicationEngineTestAccessor::GetCommittee(engine);
    ASSERT_FALSE(resolvedCommittee.empty());

    auto script = ApplicationEngineTestAccessor::CreateCommitteeMultiSigScript(engine, resolvedCommittee);
    ASSERT_FALSE(script.IsEmpty());

    auto committeeHash = cryptography::Hash::Hash160(io::ByteSpan(script.Data(), script.Size()));
    auto consensusHash = BuildConsensusHashFromScriptHash(committeeHash);

    // Simulate that the committee contract script hash is present on the invocation stack.
    ApplicationEngineTestAccessor::PushScriptHash(engine, committeeHash);

    EXPECT_TRUE(ApplicationEngineTestAccessor::IsCommitteeHash(engine, consensusHash));
    EXPECT_TRUE(ApplicationEngineTestAccessor::VerifyCommitteeConsensus(engine, consensusHash));
    EXPECT_TRUE(ApplicationEngineTestAccessor::VerifyMultiSignatureHash(engine, consensusHash));
}

TEST(ApplicationEngineCommittee, RejectsUnknownHash)
{
    ApplicationEngine engine(TriggerType::Application, nullptr, nullptr, nullptr, ApplicationEngine::TestModeGas);

    auto random =
        neo::io::UInt256::Parse("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    EXPECT_FALSE(ApplicationEngineTestAccessor::IsCommitteeHash(engine, random));
    EXPECT_FALSE(ApplicationEngineTestAccessor::VerifyCommitteeConsensus(engine, random));
    EXPECT_FALSE(ApplicationEngineTestAccessor::VerifyMultiSignatureHash(engine, random));
}
}  // namespace neo::smartcontract
