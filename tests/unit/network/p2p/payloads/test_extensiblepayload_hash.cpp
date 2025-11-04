#include <gtest/gtest.h>

#include <neo/io/byte_vector.h>
#include <neo/io/uint160.h>
#include <neo/ledger/witness.h>
#include <neo/network/p2p/payloads/extensible_payload.h>

using neo::io::ByteSpan;
using neo::io::ByteVector;
using neo::io::UInt160;
using neo::ledger::Witness;
using neo::network::p2p::payloads::ExtensiblePayload;

namespace
{
Witness MakeWitness()
{
    ByteVector invocation{0x01, 0x02};                 // minimal script content
    ByteVector verification{0x03, 0x04, 0x05, 0x06};   // placeholder verification script
    return Witness(invocation, verification);
}
}  // namespace

TEST(ExtensiblePayloadHashTest, HashChangesWhenDataChanges)
{
    const std::string category = "dbft";
    const uint32_t start = 100;
    const uint32_t end = 200;
    const UInt160 sender = UInt160::Zero();

    ByteVector data_a{0x10, 0x20, 0x30};
    ByteVector data_b{0x10, 0x20, 0x31};

    ExtensiblePayload payload_a(category, start, end, sender, data_a, MakeWitness());
    ExtensiblePayload payload_b(category, start, end, sender, data_b, MakeWitness());

    const auto hash_a = payload_a.GetHash();
    const auto hash_b = payload_b.GetHash();

    EXPECT_FALSE(hash_a.IsZero());
    EXPECT_FALSE(hash_b.IsZero());
    EXPECT_NE(hash_a, hash_b);
}

TEST(ExtensiblePayloadHashTest, HashStableAcrossCalls)
{
    ExtensiblePayload payload("consensus", 1, 2, UInt160::Zero(), ByteVector{0x01}, MakeWitness());

    const auto first = payload.GetHash();
    const auto second = payload.GetHash();

    EXPECT_EQ(first, second);
}
