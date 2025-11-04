#include <gtest/gtest.h>
#include <neo/cryptography/crypto.h>
#include <neo/cryptography/ecc/ecc_curve.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/byte_vector.h>

using namespace neo::cryptography;
using namespace neo::io;

namespace
{
neo::io::ByteVector GenerateValidPrivateKey()
{
    // Attempt until we obtain a private key that Crypto::ComputePublicKey accepts
    for (int i = 0; i < 16; ++i)
    {
        auto candidate = Crypto::GenerateRandomBytes(32);
        try
        {
            (void)Crypto::ComputePublicKey(candidate.AsSpan());
            return candidate;
        }
        catch (const std::exception&)
        {
            // Try again
        }
    }
    throw std::runtime_error("Failed to generate valid secp256r1 private key");
}
}  // namespace

TEST(CryptoSignTest, SignVerifyUsesSecp256r1ByDefault)
{
    auto privateKey = GenerateValidPrivateKey();
    auto publicKey = Crypto::ComputePublicKey(privateKey.AsSpan());

    auto message = ByteVector::FromString("neo-cpp");
    auto signature = Crypto::Sign(message.AsSpan(), privateKey.AsSpan());
    EXPECT_EQ(signature.Size(), ecc::ECCCurve::Secp256r1().GetFieldSize() * 2);

    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature.AsSpan(), publicKey));
}

TEST(CryptoSignTest, SignVerifyWithExplicitCurve)
{
    auto privateKey = GenerateValidPrivateKey();
    auto publicKey = Crypto::ComputePublicKey(privateKey.AsSpan(), ecc::ECCCurve::Secp256r1());

    auto message = ByteVector::FromString("explicit-curve");
    auto signature =
        Crypto::Sign(message.AsSpan(), privateKey.AsSpan(), ecc::ECCCurve::Secp256r1());

    EXPECT_TRUE(Crypto::VerifySignature(message.AsSpan(), signature.AsSpan(), publicKey,
                                        ecc::ECCCurve::Secp256r1()));
}
