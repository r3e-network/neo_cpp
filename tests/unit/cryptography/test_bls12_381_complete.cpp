#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "neo/cryptography/bls12_381.h"
#include "neo/cryptography/bls12_381/fp.h"
#include "neo/cryptography/bls12_381/fp2.h"
#include "neo/cryptography/bls12_381/fp6.h"
#include "neo/cryptography/bls12_381/fp12.h"
#include "neo/cryptography/bls12_381/g1.h"
#include "neo/cryptography/bls12_381/g2.h"
#include "neo/cryptography/bls12_381/gt.h"
#include "neo/cryptography/bls12_381/pairing.h"
#include <random>
#include <vector>

using namespace neo::cryptography::bls12_381;

class BLS12_381CompleteTest : public ::testing::Test {
protected:
    void SetUp() override {
        rng_.seed(42); // Fixed seed for reproducible tests
    }
    
    std::mt19937 rng_;
    
    // Test vectors from C# implementation
    const std::string fp_one_hex = "0x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001";
    const std::string fp_zero_hex = "0x000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
    const std::string fp_p_hex = "0x1a0111ea397fe69a4b1ba7b6434bacd764774b84f38512bf6730d2a0f6b0f6241eabfffeb153ffffb9feffffffffaaab";
};

// Field Element Fp Tests
TEST_F(BLS12_381CompleteTest, FpConstruction) {
    Fp zero = Fp::Zero();
    Fp one = Fp::One();
    
    EXPECT_TRUE(zero.IsZero());
    EXPECT_FALSE(one.IsZero());
    EXPECT_EQ(one.ToHexString(), fp_one_hex);
}

TEST_F(BLS12_381CompleteTest, FpArithmetic) {
    Fp a = Fp::FromInteger(5);
    Fp b = Fp::FromInteger(7);
    
    // Addition
    Fp sum = a + b;
    EXPECT_EQ(sum, Fp::FromInteger(12));
    
    // Subtraction
    Fp diff = b - a;
    EXPECT_EQ(diff, Fp::FromInteger(2));
    
    // Multiplication
    Fp prod = a * b;
    EXPECT_EQ(prod, Fp::FromInteger(35));
    
    // Division
    Fp quot = b / a;
    Fp expected = b * a.Inverse();
    EXPECT_EQ(quot, expected);
}

TEST_F(BLS12_381CompleteTest, FpSquareAndPower) {
    Fp x = Fp::FromInteger(3);
    
    // Square
    Fp x_squared = x.Square();
    EXPECT_EQ(x_squared, Fp::FromInteger(9));
    
    // Power
    Fp x_cubed = x.Pow(3);
    EXPECT_EQ(x_cubed, Fp::FromInteger(27));
    
    // Square root
    Fp sqrt_9 = x_squared.Sqrt();
    EXPECT_TRUE(sqrt_9 == x || sqrt_9 == -x);
}

TEST_F(BLS12_381CompleteTest, FpInverse) {
    Fp x = Fp::FromInteger(7);
    Fp x_inv = x.Inverse();
    
    Fp product = x * x_inv;
    EXPECT_EQ(product, Fp::One());
}

TEST_F(BLS12_381CompleteTest, FpNegation) {
    Fp x = Fp::FromInteger(5);
    Fp neg_x = -x;
    
    Fp sum = x + neg_x;
    EXPECT_EQ(sum, Fp::Zero());
}

TEST_F(BLS12_381CompleteTest, FpFromBytes) {
    std::vector<uint8_t> bytes(48, 0);
    bytes[47] = 1; // Little-endian representation of 1
    
    Fp x = Fp::FromBytes(bytes);
    EXPECT_EQ(x, Fp::One());
}

TEST_F(BLS12_381CompleteTest, FpToBytes) {
    Fp x = Fp::FromInteger(255);
    auto bytes = x.ToBytes();
    
    EXPECT_EQ(bytes.size(), 48);
    EXPECT_EQ(bytes[47], 255);
    for (size_t i = 0; i < 47; i++) {
        EXPECT_EQ(bytes[i], 0);
    }
}

// Field Element Fp2 Tests
TEST_F(BLS12_381CompleteTest, Fp2Construction) {
    Fp2 zero = Fp2::Zero();
    Fp2 one = Fp2::One();
    
    EXPECT_TRUE(zero.IsZero());
    EXPECT_FALSE(one.IsZero());
    EXPECT_EQ(one.C0(), Fp::One());
    EXPECT_EQ(one.C1(), Fp::Zero());
}

TEST_F(BLS12_381CompleteTest, Fp2Arithmetic) {
    Fp2 a(Fp::FromInteger(1), Fp::FromInteger(2));
    Fp2 b(Fp::FromInteger(3), Fp::FromInteger(4));
    
    // Addition: (1 + 2i) + (3 + 4i) = 4 + 6i
    Fp2 sum = a + b;
    EXPECT_EQ(sum.C0(), Fp::FromInteger(4));
    EXPECT_EQ(sum.C1(), Fp::FromInteger(6));
    
    // Subtraction: (3 + 4i) - (1 + 2i) = 2 + 2i
    Fp2 diff = b - a;
    EXPECT_EQ(diff.C0(), Fp::FromInteger(2));
    EXPECT_EQ(diff.C1(), Fp::FromInteger(2));
    
    // Multiplication: (1 + 2i)(3 + 4i) = 3 + 4i + 6i + 8i² = 3 + 10i - 8 = -5 + 10i
    Fp2 prod = a * b;
    // Note: i² = -1 in Fp2
}

TEST_F(BLS12_381CompleteTest, Fp2SquareAndInverse) {
    Fp2 x(Fp::FromInteger(2), Fp::FromInteger(3));
    
    // Square
    Fp2 x_squared = x.Square();
    Fp2 x_squared_alt = x * x;
    EXPECT_EQ(x_squared, x_squared_alt);
    
    // Inverse
    Fp2 x_inv = x.Inverse();
    Fp2 product = x * x_inv;
    EXPECT_EQ(product, Fp2::One());
}

TEST_F(BLS12_381CompleteTest, Fp2Conjugate) {
    Fp2 x(Fp::FromInteger(5), Fp::FromInteger(7));
    Fp2 x_conj = x.Conjugate();
    
    EXPECT_EQ(x_conj.C0(), Fp::FromInteger(5));
    EXPECT_EQ(x_conj.C1(), -Fp::FromInteger(7));
}

// Field Element Fp6 Tests
TEST_F(BLS12_381CompleteTest, Fp6Construction) {
    Fp6 zero = Fp6::Zero();
    Fp6 one = Fp6::One();
    
    EXPECT_TRUE(zero.IsZero());
    EXPECT_FALSE(one.IsZero());
}

TEST_F(BLS12_381CompleteTest, Fp6Arithmetic) {
    Fp2 a0(Fp::FromInteger(1), Fp::FromInteger(2));
    Fp2 a1(Fp::FromInteger(3), Fp::FromInteger(4));
    Fp2 a2(Fp::FromInteger(5), Fp::FromInteger(6));
    Fp6 a(a0, a1, a2);
    
    Fp2 b0(Fp::FromInteger(7), Fp::FromInteger(8));
    Fp2 b1(Fp::FromInteger(9), Fp::FromInteger(10));
    Fp2 b2(Fp::FromInteger(11), Fp::FromInteger(12));
    Fp6 b(b0, b1, b2);
    
    // Addition
    Fp6 sum = a + b;
    EXPECT_EQ(sum.C0(), a0 + b0);
    EXPECT_EQ(sum.C1(), a1 + b1);
    EXPECT_EQ(sum.C2(), a2 + b2);
    
    // Multiplication
    Fp6 prod = a * b;
    EXPECT_NE(prod, Fp6::Zero());
}

// Field Element Fp12 Tests
TEST_F(BLS12_381CompleteTest, Fp12Construction) {
    Fp12 zero = Fp12::Zero();
    Fp12 one = Fp12::One();
    
    EXPECT_TRUE(zero.IsZero());
    EXPECT_FALSE(one.IsZero());
}

TEST_F(BLS12_381CompleteTest, Fp12Arithmetic) {
    Fp12 a = Fp12::Random(rng_);
    Fp12 b = Fp12::Random(rng_);
    
    // Addition
    Fp12 sum = a + b;
    Fp12 sum_alt = b + a; // Commutativity
    EXPECT_EQ(sum, sum_alt);
    
    // Multiplication
    Fp12 prod = a * b;
    Fp12 prod_alt = b * a; // Commutativity
    EXPECT_EQ(prod, prod_alt);
    
    // Inverse
    Fp12 a_inv = a.Inverse();
    Fp12 identity = a * a_inv;
    EXPECT_EQ(identity, Fp12::One());
}

TEST_F(BLS12_381CompleteTest, Fp12Exponentiation) {
    Fp12 base = Fp12::Random(rng_);
    
    // x^0 = 1
    Fp12 result = base.Pow(0);
    EXPECT_EQ(result, Fp12::One());
    
    // x^1 = x
    result = base.Pow(1);
    EXPECT_EQ(result, base);
    
    // x^2 = x * x
    result = base.Pow(2);
    EXPECT_EQ(result, base * base);
}

// G1 Point Tests
TEST_F(BLS12_381CompleteTest, G1Construction) {
    G1 identity = G1::Identity();
    G1 generator = G1::Generator();
    
    EXPECT_TRUE(identity.IsIdentity());
    EXPECT_FALSE(generator.IsIdentity());
    EXPECT_TRUE(generator.IsOnCurve());
}

TEST_F(BLS12_381CompleteTest, G1PointArithmetic) {
    G1 g = G1::Generator();
    G1 identity = G1::Identity();
    
    // Addition with identity
    G1 sum = g + identity;
    EXPECT_EQ(sum, g);
    
    // Scalar multiplication
    G1 g2 = g * Scalar::FromInteger(2);
    G1 g2_alt = g + g;
    EXPECT_EQ(g2, g2_alt);
    
    // Negation
    G1 neg_g = -g;
    G1 zero = g + neg_g;
    EXPECT_TRUE(zero.IsIdentity());
}

TEST_F(BLS12_381CompleteTest, G1Serialization) {
    G1 g = G1::Generator();
    
    // Serialize
    auto bytes = g.ToBytes();
    EXPECT_EQ(bytes.size(), 48); // Compressed format
    
    // Deserialize
    G1 g_deserialized = G1::FromBytes(bytes);
    EXPECT_EQ(g, g_deserialized);
}

TEST_F(BLS12_381CompleteTest, G1MultiScalarMul) {
    std::vector<G1> points = {
        G1::Generator(),
        G1::Generator() * Scalar::FromInteger(2),
        G1::Generator() * Scalar::FromInteger(3)
    };
    
    std::vector<Scalar> scalars = {
        Scalar::FromInteger(1),
        Scalar::FromInteger(2),
        Scalar::FromInteger(3)
    };
    
    // 1*G + 2*(2G) + 3*(3G) = G + 4G + 9G = 14G
    G1 result = G1::MultiScalarMul(points, scalars);
    G1 expected = G1::Generator() * Scalar::FromInteger(14);
    EXPECT_EQ(result, expected);
}

// G2 Point Tests
TEST_F(BLS12_381CompleteTest, G2Construction) {
    G2 identity = G2::Identity();
    G2 generator = G2::Generator();
    
    EXPECT_TRUE(identity.IsIdentity());
    EXPECT_FALSE(generator.IsIdentity());
    EXPECT_TRUE(generator.IsOnCurve());
}

TEST_F(BLS12_381CompleteTest, G2PointArithmetic) {
    G2 g = G2::Generator();
    G2 identity = G2::Identity();
    
    // Addition
    G2 sum = g + identity;
    EXPECT_EQ(sum, g);
    
    // Scalar multiplication
    G2 g3 = g * Scalar::FromInteger(3);
    G2 g3_alt = g + g + g;
    EXPECT_EQ(g3, g3_alt);
}

TEST_F(BLS12_381CompleteTest, G2Serialization) {
    G2 g = G2::Generator();
    
    // Serialize
    auto bytes = g.ToBytes();
    EXPECT_EQ(bytes.size(), 96); // Compressed format for G2
    
    // Deserialize
    G2 g_deserialized = G2::FromBytes(bytes);
    EXPECT_EQ(g, g_deserialized);
}

// GT (Fp12) Tests for Pairing Results
TEST_F(BLS12_381CompleteTest, GTOperations) {
    GT a = GT::Random(rng_);
    GT b = GT::Random(rng_);
    
    // Multiplication
    GT prod = a * b;
    GT prod_alt = b * a;
    EXPECT_EQ(prod, prod_alt);
    
    // Exponentiation
    GT a_squared = a.Pow(2);
    GT a_squared_alt = a * a;
    EXPECT_EQ(a_squared, a_squared_alt);
}

// Pairing Tests
TEST_F(BLS12_381CompleteTest, PairingBilinearity) {
    G1 p = G1::Generator();
    G2 q = G2::Generator();
    
    Scalar a = Scalar::FromInteger(5);
    Scalar b = Scalar::FromInteger(7);
    
    // e(aP, bQ) = e(P, Q)^(ab)
    GT pairing1 = Pairing::Pair(p * a, q * b);
    GT pairing2 = Pairing::Pair(p, q).Pow(a * b);
    EXPECT_EQ(pairing1, pairing2);
    
    // e(P, Q + R) = e(P, Q) * e(P, R)
    G2 r = G2::Generator() * Scalar::FromInteger(3);
    GT pairing3 = Pairing::Pair(p, q + r);
    GT pairing4 = Pairing::Pair(p, q) * Pairing::Pair(p, r);
    EXPECT_EQ(pairing3, pairing4);
}

TEST_F(BLS12_381CompleteTest, PairingMulti) {
    std::vector<G1> g1_points = {
        G1::Generator(),
        G1::Generator() * Scalar::FromInteger(2)
    };
    
    std::vector<G2> g2_points = {
        G2::Generator(),
        G2::Generator() * Scalar::FromInteger(3)
    };
    
    // Multi-pairing
    GT result1 = Pairing::PairMulti(g1_points, g2_points);
    
    // Should equal e(G1, G2) * e(2*G1, 3*G2)
    GT result2 = Pairing::Pair(g1_points[0], g2_points[0]) * 
                 Pairing::Pair(g1_points[1], g2_points[1]);
    
    EXPECT_EQ(result1, result2);
}

TEST_F(BLS12_381CompleteTest, PairingIdentity) {
    G1 p = G1::Generator();
    G2 q = G2::Generator();
    G1 identity1 = G1::Identity();
    G2 identity2 = G2::Identity();
    
    // e(0, Q) = 1
    GT pairing1 = Pairing::Pair(identity1, q);
    EXPECT_EQ(pairing1, GT::One());
    
    // e(P, 0) = 1
    GT pairing2 = Pairing::Pair(p, identity2);
    EXPECT_EQ(pairing2, GT::One());
}

// Signature Scheme Tests
TEST_F(BLS12_381CompleteTest, BLSSignature) {
    // Generate key pair
    Scalar sk = Scalar::Random(rng_);
    G2 pk = G2::Generator() * sk;
    
    // Message to sign
    std::vector<uint8_t> message = {1, 2, 3, 4, 5};
    
    // Hash to curve
    G1 h = G1::HashToCurve(message);
    
    // Sign: σ = sk * H(m)
    G1 signature = h * sk;
    
    // Verify: e(H(m), pk) = e(σ, G2)
    GT lhs = Pairing::Pair(h, pk);
    GT rhs = Pairing::Pair(signature, G2::Generator());
    
    EXPECT_EQ(lhs, rhs);
}

TEST_F(BLS12_381CompleteTest, BLSAggregateSignature) {
    // Multiple signers
    std::vector<Scalar> sks = {
        Scalar::Random(rng_),
        Scalar::Random(rng_),
        Scalar::Random(rng_)
    };
    
    std::vector<G2> pks;
    for (const auto& sk : sks) {
        pks.push_back(G2::Generator() * sk);
    }
    
    // Common message
    std::vector<uint8_t> message = {1, 2, 3, 4, 5};
    G1 h = G1::HashToCurve(message);
    
    // Individual signatures
    std::vector<G1> signatures;
    for (const auto& sk : sks) {
        signatures.push_back(h * sk);
    }
    
    // Aggregate signature
    G1 agg_sig = signatures[0] + signatures[1] + signatures[2];
    
    // Aggregate public key
    G2 agg_pk = pks[0] + pks[1] + pks[2];
    
    // Verify aggregate signature
    GT lhs = Pairing::Pair(h, agg_pk);
    GT rhs = Pairing::Pair(agg_sig, G2::Generator());
    
    EXPECT_EQ(lhs, rhs);
}

// Edge Cases and Error Handling
TEST_F(BLS12_381CompleteTest, InvalidPointDeserialization) {
    // Invalid G1 point (not on curve)
    std::vector<uint8_t> invalid_g1(48, 0xFF);
    EXPECT_THROW(G1::FromBytes(invalid_g1), std::invalid_argument);
    
    // Invalid G2 point (not on curve)
    std::vector<uint8_t> invalid_g2(96, 0xFF);
    EXPECT_THROW(G2::FromBytes(invalid_g2), std::invalid_argument);
}

TEST_F(BLS12_381CompleteTest, ScalarModularArithmetic) {
    // Test scalar arithmetic modulo r (order of G1/G2)
    Scalar max = Scalar::FromHexString("0x73eda753299d7d483339d80809a1d80553bda402fffe5bfeffffffff00000000");
    Scalar one = Scalar::One();
    
    // max + 1 should wrap around to 0
    Scalar result = max + one;
    EXPECT_EQ(result, Scalar::Zero());
}

// Performance benchmarks (optional)
TEST_F(BLS12_381CompleteTest, DISABLED_PairingBenchmark) {
    G1 p = G1::Generator();
    G2 q = G2::Generator();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        GT result = Pairing::Pair(p, q);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "100 pairings took: " << duration.count() << " ms" << std::endl;
}