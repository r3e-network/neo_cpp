// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/ecc/test_ec_point.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_EC_POINT_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_EC_POINT_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/cryptography/ecc/ec_point.h>

namespace neo {
namespace test {

class ECPointTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for ECPoint testing - complete production implementation matching C# exactly
        
        // Create the point at infinity (identity element)
        point_at_infinity = cryptography::ecc::ECPoint::Infinity();
        
        // Create generator point (base point for secp256r1/secp256k1)
        generator_point = cryptography::ecc::ECPoint::Generator();
        
        // Known test points matching C# UT_ECPoint.cs exactly
        test_point1 = std::make_shared<cryptography::ecc::ECPoint>(
            cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromHexString("55066263022277343669578718895168534326250603453777594175500187360389116729240")),
            cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromHexString("32670510020758816978083085130507043184471273380659243275938904335757337482424"))
        );
        
        test_point2 = std::make_shared<cryptography::ecc::ECPoint>(
            cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromHexString("89565891926547004231252920425935692360644145829622209833684329913297188986597")),
            cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromHexString("12158399299693830322967808612713398636155367887041628176798871954788371653930"))
        );
        
        test_point3 = std::make_shared<cryptography::ecc::ECPoint>(
            cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromHexString("112711660439710606056748659173929673102114977341539408544630613555209775888121")),
            cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromHexString("25583027980570883691656905877401976406448868254816295069919888960541586679410"))
        );
        
        // Create some compressed and uncompressed point test data
        // These are actual points on the secp256k1 curve
        compressed_point_data = io::ByteVector::Parse("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798");
        uncompressed_point_data = io::ByteVector::Parse("0479be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8");
        
        // Invalid point data for error testing
        invalid_point_data = io::ByteVector::Parse("0299999999999999999999999999999999999999999999999999999999999999");
        
        // Generate random scalars for multiplication testing
        random_scalars.clear();
        for (int i = 0; i < 20; ++i) {
            random_scalars.push_back(cryptography::BigInteger::Random(256));
        }
        
        // Small scalar values for basic testing
        small_scalars = {
            cryptography::BigInteger::Zero(),
            cryptography::BigInteger::One(),
            cryptography::BigInteger::FromInt64(2),
            cryptography::BigInteger::FromInt64(3),
            cryptography::BigInteger::FromInt64(10),
            cryptography::BigInteger::FromInt64(255),
            cryptography::BigInteger::FromInt64(256),
            cryptography::BigInteger::FromInt64(65537)
        };
        
        // Large scalars near curve order for edge case testing
        curve_order = cryptography::BigInteger::FromHexString("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141");
        large_scalars = {
            curve_order - cryptography::BigInteger::One(),
            curve_order - cryptography::BigInteger::FromInt64(2),
            curve_order - cryptography::BigInteger::FromInt64(100)
        };
        
        // Pre-computed multiples of generator for verification
        generator_multiples.clear();
        auto current_point = point_at_infinity;
        for (int i = 0; i < 10; ++i) {
            generator_multiples.push_back(std::make_shared<cryptography::ecc::ECPoint>(current_point));
            current_point = current_point + *generator_point;
        }
    }

    void TearDown() override {
        // Clean up test fixtures - ensure no memory leaks
        point_at_infinity.reset();
        generator_point.reset();
        test_point1.reset();
        test_point2.reset();
        test_point3.reset();
        random_scalars.clear();
        small_scalars.clear();
        large_scalars.clear();
        generator_multiples.clear();
    }

    // Helper methods and test data for complete ECPoint testing
    std::shared_ptr<cryptography::ecc::ECPoint> point_at_infinity;
    std::shared_ptr<cryptography::ecc::ECPoint> generator_point;
    std::shared_ptr<cryptography::ecc::ECPoint> test_point1;
    std::shared_ptr<cryptography::ecc::ECPoint> test_point2;
    std::shared_ptr<cryptography::ecc::ECPoint> test_point3;
    
    // Serialization test data
    io::ByteVector compressed_point_data;
    io::ByteVector uncompressed_point_data;
    io::ByteVector invalid_point_data;
    
    // Scalar multiplication test data
    std::vector<cryptography::BigInteger> random_scalars;
    std::vector<cryptography::BigInteger> small_scalars;
    std::vector<cryptography::BigInteger> large_scalars;
    cryptography::BigInteger curve_order;
    
    // Pre-computed test data
    std::vector<std::shared_ptr<cryptography::ecc::ECPoint>> generator_multiples;
    
    // Helper method to verify point is on curve
    bool IsOnCurve(const cryptography::ecc::ECPoint& point) {
        if (point.IsInfinity()) return true;
        
        auto x = point.GetX();
        auto y = point.GetY();
        
        // For secp256k1: y² = x³ + 7
        auto y_squared = y.Square();
        auto x_cubed = x.Square() * x;
        auto seven = cryptography::ecc::ECFieldElement(cryptography::BigInteger::FromInt64(7));
        auto rhs = x_cubed + seven;
        
        return y_squared == rhs;
    }
    
    // Helper method to create random valid point
    std::shared_ptr<cryptography::ecc::ECPoint> CreateRandomPoint() {
        auto random_scalar = cryptography::BigInteger::Random(256);
        return std::make_shared<cryptography::ecc::ECPoint>(*generator_point * random_scalar);
    }
    
    // Helper method to verify point multiplication properties
    bool VerifyScalarMultiplication(const cryptography::ecc::ECPoint& P, const cryptography::BigInteger& k) {
        auto result = P * k;
        
        // k*P should be on curve
        if (!IsOnCurve(result)) return false;
        
        // 0*P should be infinity
        if (k == cryptography::BigInteger::Zero()) {
            return result.IsInfinity();
        }
        
        // 1*P should equal P
        if (k == cryptography::BigInteger::One()) {
            return result == P;
        }
        
        return true;
    }
};

// Complete ECPoint test methods - production-ready implementation matching C# UT_ECPoint.cs exactly

TEST_F(ECPointTest, InfinityPointCreatedCorrectly) {
    EXPECT_NE(point_at_infinity, nullptr);
    EXPECT_TRUE(point_at_infinity->IsInfinity());
    EXPECT_TRUE(IsOnCurve(*point_at_infinity));
}

TEST_F(ECPointTest, GeneratorPointIsValid) {
    EXPECT_NE(generator_point, nullptr);
    EXPECT_FALSE(generator_point->IsInfinity());
    EXPECT_TRUE(IsOnCurve(*generator_point));
    
    // Generator should be the standard secp256k1 generator point
    auto expected_x = cryptography::ecc::ECFieldElement(
        cryptography::BigInteger::FromHexString("55066263022277343669578718895168534326250603453777594175500187360389116729240")
    );
    auto expected_y = cryptography::ecc::ECFieldElement(
        cryptography::BigInteger::FromHexString("32670510020758816978083085130507043184471273380659243275938904335757337482424")
    );
    
    EXPECT_TRUE(generator_point->GetX() == expected_x);
    EXPECT_TRUE(generator_point->GetY() == expected_y);
}

TEST_F(ECPointTest, TestPointsAreOnCurve) {
    EXPECT_TRUE(IsOnCurve(*test_point1));
    EXPECT_TRUE(IsOnCurve(*test_point2));
    EXPECT_TRUE(IsOnCurve(*test_point3));
    
    // All test points should be finite (not infinity)
    EXPECT_FALSE(test_point1->IsInfinity());
    EXPECT_FALSE(test_point2->IsInfinity());
    EXPECT_FALSE(test_point3->IsInfinity());
}

TEST_F(ECPointTest, PointAdditionWorksCorrectly) {
    // Test identity: P + O = P (where O is point at infinity)
    auto p_plus_infinity = *test_point1 + *point_at_infinity;
    EXPECT_TRUE(p_plus_infinity == *test_point1);
    
    auto infinity_plus_p = *point_at_infinity + *test_point1;
    EXPECT_TRUE(infinity_plus_p == *test_point1);
    
    // Test commutativity: P + Q = Q + P
    auto pq = *test_point1 + *test_point2;
    auto qp = *test_point2 + *test_point1;
    EXPECT_TRUE(pq == qp);
    EXPECT_TRUE(IsOnCurve(pq));
    
    // Test associativity: (P + Q) + R = P + (Q + R)
    auto pqr1 = (*test_point1 + *test_point2) + *test_point3;
    auto pqr2 = *test_point1 + (*test_point2 + *test_point3);
    EXPECT_TRUE(pqr1 == pqr2);
    EXPECT_TRUE(IsOnCurve(pqr1));
}

TEST_F(ECPointTest, PointDoubling) {
    // Test point doubling: 2P = P + P
    auto doubled = *test_point1 + *test_point1;
    auto doubled_direct = test_point1->Double();
    
    EXPECT_TRUE(doubled == doubled_direct);
    EXPECT_TRUE(IsOnCurve(doubled));
    
    // Test doubling generator point
    auto gen_doubled = generator_point->Double();
    EXPECT_TRUE(IsOnCurve(gen_doubled));
    EXPECT_FALSE(gen_doubled.IsInfinity());
}

TEST_F(ECPointTest, PointNegation) {
    // Test negation: P + (-P) = O
    auto neg_point1 = test_point1->Negate();
    auto sum_with_neg = *test_point1 + neg_point1;
    
    EXPECT_TRUE(sum_with_neg.IsInfinity());
    EXPECT_TRUE(IsOnCurve(neg_point1));
    
    // Test double negation: -(-P) = P
    auto double_neg = neg_point1.Negate();
    EXPECT_TRUE(double_neg == *test_point1);
    
    // Negation of infinity should be infinity
    auto neg_infinity = point_at_infinity->Negate();
    EXPECT_TRUE(neg_infinity.IsInfinity());
}

TEST_F(ECPointTest, ScalarMultiplicationSmallValues) {
    // Test scalar multiplication with small values
    for (const auto& scalar : small_scalars) {
        auto result = *generator_point * scalar;
        EXPECT_TRUE(VerifyScalarMultiplication(*generator_point, scalar));
        EXPECT_TRUE(IsOnCurve(result));
        
        // Test with test points
        auto result2 = *test_point1 * scalar;
        EXPECT_TRUE(VerifyScalarMultiplication(*test_point1, scalar));
        EXPECT_TRUE(IsOnCurve(result2));
    }
}

TEST_F(ECPointTest, ScalarMultiplicationLargeValues) {
    // Test scalar multiplication with large values
    for (const auto& scalar : large_scalars) {
        auto result = *generator_point * scalar;
        EXPECT_TRUE(IsOnCurve(result));
        
        // Large scalars near curve order should still produce valid points
        EXPECT_FALSE(result.IsInfinity()); // Should not be infinity for these specific scalars
    }
}

TEST_F(ECPointTest, ScalarMultiplicationRandomValues) {
    // Test scalar multiplication with random values
    for (const auto& scalar : random_scalars) {
        auto result = *generator_point * scalar;
        EXPECT_TRUE(IsOnCurve(result));
        
        // Test linearity: k(P + Q) = kP + kQ
        auto p_plus_q = *test_point1 + *test_point2;
        auto k_p_plus_q = p_plus_q * scalar;
        auto kp_plus_kq = (*test_point1 * scalar) + (*test_point2 * scalar);
        
        EXPECT_TRUE(k_p_plus_q == kp_plus_kq);
    }
}

TEST_F(ECPointTest, ScalarMultiplicationDistributivity) {
    // Test distributivity: (a + b)P = aP + bP
    auto a = cryptography::BigInteger::FromInt64(123);
    auto b = cryptography::BigInteger::FromInt64(456);
    auto a_plus_b = a + b;
    
    auto ab_p = *test_point1 * a_plus_b;
    auto ap_plus_bp = (*test_point1 * a) + (*test_point1 * b);
    
    EXPECT_TRUE(ab_p == ap_plus_bp);
}

TEST_F(ECPointTest, ScalarMultiplicationAssociativity) {
    // Test associativity: (ab)P = a(bP)
    auto a = cryptography::BigInteger::FromInt64(17);
    auto b = cryptography::BigInteger::FromInt64(31);
    auto ab = a * b;
    
    auto ab_p = *test_point1 * ab;
    auto bp = *test_point1 * b;
    auto a_bp = bp * a;
    
    EXPECT_TRUE(ab_p == a_bp);
}

TEST_F(ECPointTest, CurveOrderMultiplication) {
    // Test that multiplying by curve order gives infinity
    auto result = *generator_point * curve_order;
    EXPECT_TRUE(result.IsInfinity());
    
    // Test with other points
    auto result2 = *test_point1 * curve_order;
    EXPECT_TRUE(result2.IsInfinity());
}

TEST_F(ECPointTest, CompressedPointSerialization) {
    // Test serialization to compressed format
    auto compressed = test_point1->ToCompressedBytes();
    EXPECT_EQ(compressed.Size(), 33); // 1 byte prefix + 32 bytes x-coordinate
    
    // First byte should be 0x02 or 0x03
    EXPECT_TRUE(compressed[0] == 0x02 || compressed[0] == 0x03);
    
    // Test deserialization
    auto deserialized = cryptography::ecc::ECPoint::FromCompressedBytes(compressed);
    EXPECT_TRUE(deserialized == *test_point1);
    EXPECT_TRUE(IsOnCurve(deserialized));
}

TEST_F(ECPointTest, UncompressedPointSerialization) {
    // Test serialization to uncompressed format
    auto uncompressed = test_point1->ToUncompressedBytes();
    EXPECT_EQ(uncompressed.Size(), 65); // 1 byte prefix + 32 bytes x + 32 bytes y
    EXPECT_EQ(uncompressed[0], 0x04); // Uncompressed prefix
    
    // Test deserialization
    auto deserialized = cryptography::ecc::ECPoint::FromUncompressedBytes(uncompressed);
    EXPECT_TRUE(deserialized == *test_point1);
    EXPECT_TRUE(IsOnCurve(deserialized));
}

TEST_F(ECPointTest, SerializationRoundTrip) {
    // Test compressed round trip
    auto compressed = test_point1->ToCompressedBytes();
    auto from_compressed = cryptography::ecc::ECPoint::FromCompressedBytes(compressed);
    EXPECT_TRUE(from_compressed == *test_point1);
    
    // Test uncompressed round trip
    auto uncompressed = test_point1->ToUncompressedBytes();
    auto from_uncompressed = cryptography::ecc::ECPoint::FromUncompressedBytes(uncompressed);
    EXPECT_TRUE(from_uncompressed == *test_point1);
    
    // Both should give same result
    EXPECT_TRUE(from_compressed == from_uncompressed);
}

TEST_F(ECPointTest, InvalidPointDeserialization) {
    // Test deserialization of invalid point data
    EXPECT_THROW(
        cryptography::ecc::ECPoint::FromCompressedBytes(invalid_point_data),
        std::invalid_argument
    );
    
    // Test with wrong size data
    io::ByteVector wrong_size_data = io::ByteVector::Parse("0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f817");
    EXPECT_THROW(
        cryptography::ecc::ECPoint::FromCompressedBytes(wrong_size_data),
        std::invalid_argument
    );
}

TEST_F(ECPointTest, InfinityPointSerialization) {
    // Test infinity point serialization
    auto inf_compressed = point_at_infinity->ToCompressedBytes();
    auto inf_uncompressed = point_at_infinity->ToUncompressedBytes();
    
    // Infinity should serialize to all zeros
    EXPECT_TRUE(std::all_of(inf_compressed.begin(), inf_compressed.end(), [](uint8_t b) { return b == 0; }));
    EXPECT_TRUE(std::all_of(inf_uncompressed.begin(), inf_uncompressed.end(), [](uint8_t b) { return b == 0; }));
    
    // Test deserialization
    auto from_compressed_inf = cryptography::ecc::ECPoint::FromCompressedBytes(inf_compressed);
    auto from_uncompressed_inf = cryptography::ecc::ECPoint::FromUncompressedBytes(inf_uncompressed);
    
    EXPECT_TRUE(from_compressed_inf.IsInfinity());
    EXPECT_TRUE(from_uncompressed_inf.IsInfinity());
}

TEST_F(ECPointTest, EqualityAndComparison) {
    // Test equality
    EXPECT_TRUE(*test_point1 == *test_point1);
    EXPECT_FALSE(*test_point1 == *test_point2);
    EXPECT_TRUE(*point_at_infinity == *point_at_infinity);
    
    // Test inequality
    EXPECT_FALSE(*test_point1 != *test_point1);
    EXPECT_TRUE(*test_point1 != *test_point2);
    
    // Create another point with same coordinates
    auto same_point = std::make_shared<cryptography::ecc::ECPoint>(
        test_point1->GetX(), test_point1->GetY()
    );
    EXPECT_TRUE(*test_point1 == *same_point);
}

TEST_F(ECPointTest, HashCode) {
    // Hash codes should be consistent
    auto hash1 = test_point1->GetHashCode();
    auto hash2 = test_point1->GetHashCode();
    EXPECT_EQ(hash1, hash2);
    
    // Equal points should have equal hash codes
    auto same_point = std::make_shared<cryptography::ecc::ECPoint>(
        test_point1->GetX(), test_point1->GetY()
    );
    EXPECT_EQ(test_point1->GetHashCode(), same_point->GetHashCode());
    
    // Different points should likely have different hash codes
    EXPECT_NE(test_point1->GetHashCode(), test_point2->GetHashCode());
}

TEST_F(ECPointTest, ToStringRepresentation) {
    // Test string representation
    std::string point_str = test_point1->ToString();
    EXPECT_FALSE(point_str.empty());
    
    std::string inf_str = point_at_infinity->ToString();
    EXPECT_FALSE(inf_str.empty());
    
    // Should contain coordinate information for finite points
    EXPECT_NE(point_str.find("x"), std::string::npos);
    EXPECT_NE(point_str.find("y"), std::string::npos);
}

TEST_F(ECPointTest, GeneratorMultiples) {
    // Test pre-computed generator multiples
    for (size_t i = 0; i < generator_multiples.size(); ++i) {
        auto expected = *generator_point * cryptography::BigInteger::FromInt64(i);
        EXPECT_TRUE(*generator_multiples[i] == expected);
        EXPECT_TRUE(IsOnCurve(*generator_multiples[i]));
    }
}

TEST_F(ECPointTest, PerformanceScalarMultiplication) {
    // Test performance of scalar multiplication
    auto large_scalar = cryptography::BigInteger::Random(256);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto result = *generator_point * large_scalar;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    EXPECT_TRUE(IsOnCurve(result));
    EXPECT_LT(duration.count(), 100); // Should complete within 100ms
}

TEST_F(ECPointTest, BatchOperations) {
    // Test batch point operations
    std::vector<cryptography::ecc::ECPoint> points;
    for (int i = 1; i <= 10; ++i) {
        points.push_back(*generator_point * cryptography::BigInteger::FromInt64(i));
    }
    
    // All points should be on curve
    for (const auto& point : points) {
        EXPECT_TRUE(IsOnCurve(point));
        EXPECT_FALSE(point.IsInfinity());
    }
    
    // Test sum of points
    auto sum = *point_at_infinity;
    for (const auto& point : points) {
        sum = sum + point;
        EXPECT_TRUE(IsOnCurve(sum));
    }
}

TEST_F(ECPointTest, EdgeCaseArithmetic) {
    // Test adding point to itself multiple times
    auto current = *test_point1;
    for (int i = 1; i < 10; ++i) {
        current = current + *test_point1;
        EXPECT_TRUE(IsOnCurve(current));
        
        // Should equal scalar multiplication
        auto scalar_result = *test_point1 * cryptography::BigInteger::FromInt64(i + 1);
        EXPECT_TRUE(current == scalar_result);
    }
}

TEST_F(ECPointTest, ValidateKnownTestVectors) {
    // Test with known compressed point
    auto point_from_compressed = cryptography::ecc::ECPoint::FromCompressedBytes(compressed_point_data);
    EXPECT_TRUE(IsOnCurve(point_from_compressed));
    EXPECT_FALSE(point_from_compressed.IsInfinity());
    
    // Test with known uncompressed point
    auto point_from_uncompressed = cryptography::ecc::ECPoint::FromUncompressedBytes(uncompressed_point_data);
    EXPECT_TRUE(IsOnCurve(point_from_uncompressed));
    EXPECT_FALSE(point_from_uncompressed.IsInfinity());
    
    // Both should represent the same point
    EXPECT_TRUE(point_from_compressed == point_from_uncompressed);
}

TEST_F(ECPointTest, CopyConstructorAndAssignment) {
    // Test copy constructor
    cryptography::ecc::ECPoint copied(*test_point1);
    EXPECT_TRUE(copied == *test_point1);
    EXPECT_TRUE(IsOnCurve(copied));
    
    // Test assignment operator
    cryptography::ecc::ECPoint assigned = *test_point2;
    EXPECT_TRUE(assigned == *test_point2);
    EXPECT_TRUE(IsOnCurve(assigned));
    
    // Test self-assignment safety
    assigned = assigned;
    EXPECT_TRUE(assigned == *test_point2);
}

TEST_F(ECPointTest, RandomPointsArithmetic) {
    // Generate random points and test arithmetic
    for (int i = 0; i < 10; ++i) {
        auto random_point1 = CreateRandomPoint();
        auto random_point2 = CreateRandomPoint();
        
        EXPECT_TRUE(IsOnCurve(*random_point1));
        EXPECT_TRUE(IsOnCurve(*random_point2));
        
        // Test addition
        auto sum = *random_point1 + *random_point2;
        EXPECT_TRUE(IsOnCurve(sum));
        
        // Test scalar multiplication
        auto scalar = cryptography::BigInteger::FromInt64(42);
        auto scaled = *random_point1 * scalar;
        EXPECT_TRUE(IsOnCurve(scaled));
    }
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_EC_POINT_CPP_H
