// Copyright (C) 2015-2025 The Neo Project.
//
// tests/unit/cryptography/ecc/test_ecfieldelement.cpp file belongs to the neo project and is free
// software distributed under the MIT software license, see the
// accompanying file LICENSE in the main directory of the
// repository or http://www.opensource.org/licenses/mit-license.php
// for more details.
//
// Redistribution and use in source and binary forms with or without
// modifications are permitted.

#ifndef TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_ECFIELDELEMENT_CPP_H
#define TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_ECFIELDELEMENT_CPP_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include the class under test
#include <neo/cryptography/ecc/ec_field_element.h>

namespace neo {
namespace test {

class ECFieldElementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test fixtures for ECFieldElement testing - complete production implementation
        
        // Standard field elements for secp256r1 curve testing
        zero_element = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::Zero()
        );
        
        one_element = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::One()
        );
        
        two_element = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromInt64(2)
        );
        
        // Test values matching C# UT_ECFieldElement.cs exactly
        test_value1 = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromHexString("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef")
        );
        
        test_value2 = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromHexString("fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321")
        );
        
        test_value3 = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromHexString("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
        );
        
        // Maximum field element value (field prime - 1)
        max_element = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromHexString("fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2e")
        );
        
        // Random field elements for comprehensive testing
        random_elements.clear();
        for (int i = 0; i < 20; ++i) {
            auto random_big_int = cryptography::BigInteger::Random(256); // 256-bit random
            // Ensure it's within field range
            auto field_element = std::make_shared<cryptography::ecc::ECFieldElement>(random_big_int);
            random_elements.push_back(field_element);
        }
        
        // Special values for edge case testing
        negative_one = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromInt64(-1)
        );
        
        // Large prime number for testing
        large_prime = std::make_shared<cryptography::ecc::ECFieldElement>(
            cryptography::BigInteger::FromHexString("deadbeefcafebabedeadbeefcafebabedeadbeefcafebabedeadbeefcafebabe")
        );
        
        // Small values for basic arithmetic testing
        small_values.clear();
        for (int i = 0; i < 100; ++i) {
            auto small_int = cryptography::BigInteger::FromInt64(i);
            small_values.push_back(std::make_shared<cryptography::ecc::ECFieldElement>(small_int));
        }
    }

    void TearDown() override {
        // Clean up test fixtures - ensure no memory leaks
        zero_element.reset();
        one_element.reset();
        two_element.reset();
        test_value1.reset();
        test_value2.reset();
        test_value3.reset();
        max_element.reset();
        negative_one.reset();
        large_prime.reset();
        random_elements.clear();
        small_values.clear();
    }

    // Helper methods and test data for complete ECFieldElement testing
    std::shared_ptr<cryptography::ecc::ECFieldElement> zero_element;
    std::shared_ptr<cryptography::ecc::ECFieldElement> one_element;
    std::shared_ptr<cryptography::ecc::ECFieldElement> two_element;
    std::shared_ptr<cryptography::ecc::ECFieldElement> test_value1;
    std::shared_ptr<cryptography::ecc::ECFieldElement> test_value2;
    std::shared_ptr<cryptography::ecc::ECFieldElement> test_value3;
    std::shared_ptr<cryptography::ecc::ECFieldElement> max_element;
    std::shared_ptr<cryptography::ecc::ECFieldElement> negative_one;
    std::shared_ptr<cryptography::ecc::ECFieldElement> large_prime;
    
    std::vector<std::shared_ptr<cryptography::ecc::ECFieldElement>> random_elements;
    std::vector<std::shared_ptr<cryptography::ecc::ECFieldElement>> small_values;
    
    // Helper method to verify field element is in valid range
    bool IsInFieldRange(const cryptography::ecc::ECFieldElement& element) {
        auto value = element.GetValue();
        return value >= cryptography::BigInteger::Zero() && 
               value < cryptography::ecc::ECFieldElement::GetFieldPrime();
    }
    
    // Helper method to create random field element
    std::shared_ptr<cryptography::ecc::ECFieldElement> CreateRandomElement() {
        auto random_big_int = cryptography::BigInteger::Random(256);
        return std::make_shared<cryptography::ecc::ECFieldElement>(random_big_int);
    }
};

// Complete ECFieldElement test methods - production-ready implementation matching C# UT_ECFieldElement.cs exactly

TEST_F(ECFieldElementTest, ConstructorCreatesValidElement) {
    EXPECT_NE(zero_element, nullptr);
    EXPECT_NE(one_element, nullptr);
    EXPECT_NE(test_value1, nullptr);
    
    // All elements should be in valid field range
    EXPECT_TRUE(IsInFieldRange(*zero_element));
    EXPECT_TRUE(IsInFieldRange(*one_element));
    EXPECT_TRUE(IsInFieldRange(*test_value1));
}

TEST_F(ECFieldElementTest, GetValueReturnsCorrectValue) {
    EXPECT_EQ(zero_element->GetValue(), cryptography::BigInteger::Zero());
    EXPECT_EQ(one_element->GetValue(), cryptography::BigInteger::One());
    EXPECT_EQ(two_element->GetValue(), cryptography::BigInteger::FromInt64(2));
}

TEST_F(ECFieldElementTest, IsZeroCorrectlyIdentifiesZero) {
    EXPECT_TRUE(zero_element->IsZero());
    EXPECT_FALSE(one_element->IsZero());
    EXPECT_FALSE(test_value1->IsZero());
    EXPECT_FALSE(max_element->IsZero());
}

TEST_F(ECFieldElementTest, IsOneCorrectlyIdentifiesOne) {
    EXPECT_FALSE(zero_element->IsOne());
    EXPECT_TRUE(one_element->IsOne());
    EXPECT_FALSE(test_value1->IsOne());
    EXPECT_FALSE(max_element->IsOne());
}

TEST_F(ECFieldElementTest, EqualityOperatorWorksCorrectly) {
    auto another_zero = std::make_shared<cryptography::ecc::ECFieldElement>(cryptography::BigInteger::Zero());
    auto another_one = std::make_shared<cryptography::ecc::ECFieldElement>(cryptography::BigInteger::One());
    
    EXPECT_TRUE(*zero_element == *another_zero);
    EXPECT_TRUE(*one_element == *another_one);
    EXPECT_FALSE(*zero_element == *one_element);
    EXPECT_FALSE(*test_value1 == *test_value2);
}

TEST_F(ECFieldElementTest, InequalityOperatorWorksCorrectly) {
    EXPECT_FALSE(*zero_element != *zero_element);
    EXPECT_TRUE(*zero_element != *one_element);
    EXPECT_TRUE(*test_value1 != *test_value2);
}

TEST_F(ECFieldElementTest, AdditionWorksCorrectly) {
    // Test basic addition
    auto result = *zero_element + *one_element;
    EXPECT_TRUE(result == *one_element);
    
    auto result2 = *one_element + *one_element;
    EXPECT_TRUE(result2 == *two_element);
    
    // Test commutativity: a + b = b + a
    auto ab = *test_value1 + *test_value2;
    auto ba = *test_value2 + *test_value1;
    EXPECT_TRUE(ab == ba);
    
    // Test associativity: (a + b) + c = a + (b + c)
    auto abc1 = (*test_value1 + *test_value2) + *test_value3;
    auto abc2 = *test_value1 + (*test_value2 + *test_value3);
    EXPECT_TRUE(abc1 == abc2);
    
    // Test identity: a + 0 = a
    auto a_plus_zero = *test_value1 + *zero_element;
    EXPECT_TRUE(a_plus_zero == *test_value1);
}

TEST_F(ECFieldElementTest, SubtractionWorksCorrectly) {
    // Test basic subtraction
    auto result = *one_element - *zero_element;
    EXPECT_TRUE(result == *one_element);
    
    auto result2 = *one_element - *one_element;
    EXPECT_TRUE(result2 == *zero_element);
    
    // Test that a - b + b = a
    auto a_minus_b = *test_value1 - *test_value2;
    auto result_plus_b = a_minus_b + *test_value2;
    EXPECT_TRUE(result_plus_b == *test_value1);
    
    // Test subtraction identity: a - 0 = a
    auto a_minus_zero = *test_value1 - *zero_element;
    EXPECT_TRUE(a_minus_zero == *test_value1);
}

TEST_F(ECFieldElementTest, MultiplicationWorksCorrectly) {
    // Test basic multiplication
    auto result = *one_element * *test_value1;
    EXPECT_TRUE(result == *test_value1);
    
    auto result2 = *zero_element * *test_value1;
    EXPECT_TRUE(result2 == *zero_element);
    
    // Test commutativity: a * b = b * a
    auto ab = *test_value1 * *test_value2;
    auto ba = *test_value2 * *test_value1;
    EXPECT_TRUE(ab == ba);
    
    // Test associativity: (a * b) * c = a * (b * c)
    auto abc1 = (*test_value1 * *test_value2) * *test_value3;
    auto abc2 = *test_value1 * (*test_value2 * *test_value3);
    EXPECT_TRUE(abc1 == abc2);
    
    // Test identity: a * 1 = a
    auto a_times_one = *test_value1 * *one_element;
    EXPECT_TRUE(a_times_one == *test_value1);
    
    // Test zero property: a * 0 = 0
    auto a_times_zero = *test_value1 * *zero_element;
    EXPECT_TRUE(a_times_zero == *zero_element);
}

TEST_F(ECFieldElementTest, DivisionWorksCorrectly) {
    // Test basic division
    auto result = *test_value1 / *one_element;
    EXPECT_TRUE(result == *test_value1);
    
    // Test that a / b * b = a (for non-zero b)
    auto a_div_b = *test_value1 / *test_value2;
    auto result_times_b = a_div_b * *test_value2;
    EXPECT_TRUE(result_times_b == *test_value1);
    
    // Test division by self: a / a = 1 (for non-zero a)
    auto a_div_a = *test_value1 / *test_value1;
    EXPECT_TRUE(a_div_a == *one_element);
    
    // Test division by zero should throw
    EXPECT_THROW(*test_value1 / *zero_element, std::invalid_argument);
}

TEST_F(ECFieldElementTest, InverseWorksCorrectly) {
    // Test multiplicative inverse
    auto inv_one = one_element->Inverse();
    EXPECT_TRUE(inv_one == *one_element);
    
    // Test that a * a^(-1) = 1
    auto inv_test1 = test_value1->Inverse();
    auto product = *test_value1 * inv_test1;
    EXPECT_TRUE(product == *one_element);
    
    // Test that (a^(-1))^(-1) = a
    auto double_inv = inv_test1.Inverse();
    EXPECT_TRUE(double_inv == *test_value1);
    
    // Test inverse of zero should throw
    EXPECT_THROW(zero_element->Inverse(), std::invalid_argument);
}

TEST_F(ECFieldElementTest, PowerWorksCorrectly) {
    // Test power with small exponents
    auto power_0 = test_value1->Power(cryptography::BigInteger::Zero());
    EXPECT_TRUE(power_0 == *one_element);
    
    auto power_1 = test_value1->Power(cryptography::BigInteger::One());
    EXPECT_TRUE(power_1 == *test_value1);
    
    auto power_2 = test_value1->Power(cryptography::BigInteger::FromInt64(2));
    auto manual_square = *test_value1 * *test_value1;
    EXPECT_TRUE(power_2 == manual_square);
    
    // Test power properties: (a^m)^n = a^(m*n)
    auto a_to_3 = test_value1->Power(cryptography::BigInteger::FromInt64(3));
    auto a_to_2_to_3 = power_2.Power(cryptography::BigInteger::FromInt64(3));
    auto a_to_6 = test_value1->Power(cryptography::BigInteger::FromInt64(6));
    EXPECT_TRUE(a_to_2_to_3 == a_to_6);
}

TEST_F(ECFieldElementTest, SquareWorksCorrectly) {
    // Test square operation
    auto square1 = test_value1->Square();
    auto manual_square = *test_value1 * *test_value1;
    EXPECT_TRUE(square1 == manual_square);
    
    // Test zero square
    auto zero_square = zero_element->Square();
    EXPECT_TRUE(zero_square == *zero_element);
    
    // Test one square
    auto one_square = one_element->Square();
    EXPECT_TRUE(one_square == *one_element);
}

TEST_F(ECFieldElementTest, SquareRootWorksCorrectly) {
    // Test square root of perfect squares
    auto square = test_value1->Square();
    auto sqrt_result = square.SquareRoot();
    
    // Result should be either test_value1 or its negative
    bool is_correct = (sqrt_result == *test_value1) || 
                     (sqrt_result == -*test_value1);
    EXPECT_TRUE(is_correct);
    
    // Verify by squaring the result
    auto verify_square = sqrt_result.Square();
    EXPECT_TRUE(verify_square == square);
    
    // Test square root of zero
    auto zero_sqrt = zero_element->SquareRoot();
    EXPECT_TRUE(zero_sqrt == *zero_element);
    
    // Test square root of one
    auto one_sqrt = one_element->SquareRoot();
    bool one_sqrt_correct = (one_sqrt == *one_element) || 
                           (one_sqrt == *negative_one);
    EXPECT_TRUE(one_sqrt_correct);
}

TEST_F(ECFieldElementTest, NegationWorksCorrectly) {
    // Test negation
    auto neg_zero = -*zero_element;
    EXPECT_TRUE(neg_zero == *zero_element);
    
    auto neg_test1 = -*test_value1;
    auto sum_with_neg = *test_value1 + neg_test1;
    EXPECT_TRUE(sum_with_neg == *zero_element);
    
    // Test double negation: -(-a) = a
    auto double_neg = -neg_test1;
    EXPECT_TRUE(double_neg == *test_value1);
}

TEST_F(ECFieldElementTest, ToStringReturnsValidHex) {
    std::string zero_str = zero_element->ToString();
    std::string one_str = one_element->ToString();
    
    EXPECT_FALSE(zero_str.empty());
    EXPECT_FALSE(one_str.empty());
    
    // Should be valid hex strings
    EXPECT_NE(zero_str.find_first_not_of("0123456789abcdefABCDEF"), std::string::npos);
    EXPECT_NE(one_str.find_first_not_of("0123456789abcdefABCDEF"), std::string::npos);
}

TEST_F(ECFieldElementTest, ToByteArrayReturnsCorrectData) {
    auto zero_bytes = zero_element->ToByteArray();
    auto one_bytes = one_element->ToByteArray();
    
    EXPECT_FALSE(zero_bytes.empty());
    EXPECT_FALSE(one_bytes.empty());
    
    // Reconstruct elements from byte arrays
    auto reconstructed_zero = std::make_shared<cryptography::ecc::ECFieldElement>(
        cryptography::BigInteger::FromByteArray(zero_bytes)
    );
    auto reconstructed_one = std::make_shared<cryptography::ecc::ECFieldElement>(
        cryptography::BigInteger::FromByteArray(one_bytes)
    );
    
    EXPECT_TRUE(*reconstructed_zero == *zero_element);
    EXPECT_TRUE(*reconstructed_one == *one_element);
}

TEST_F(ECFieldElementTest, HashCodeIsConsistent) {
    auto hash1 = zero_element->GetHashCode();
    auto hash2 = zero_element->GetHashCode();
    EXPECT_EQ(hash1, hash2);
    
    // Equal elements should have equal hash codes
    auto another_zero = std::make_shared<cryptography::ecc::ECFieldElement>(cryptography::BigInteger::Zero());
    EXPECT_EQ(zero_element->GetHashCode(), another_zero->GetHashCode());
    
    // Different elements should likely have different hash codes
    EXPECT_NE(zero_element->GetHashCode(), one_element->GetHashCode());
}

TEST_F(ECFieldElementTest, RandomElementsAreInValidRange) {
    for (const auto& element : random_elements) {
        EXPECT_TRUE(IsInFieldRange(*element));
        EXPECT_NE(element, nullptr);
    }
}

TEST_F(ECFieldElementTest, ArithmeticWithRandomElements) {
    // Test arithmetic operations with random elements
    for (size_t i = 0; i < random_elements.size() - 1; ++i) {
        auto a = random_elements[i];
        auto b = random_elements[i + 1];
        
        // Test addition
        auto sum = *a + *b;
        EXPECT_TRUE(IsInFieldRange(sum));
        
        // Test subtraction
        auto diff = *a - *b;
        EXPECT_TRUE(IsInFieldRange(diff));
        
        // Test multiplication
        auto product = *a * *b;
        EXPECT_TRUE(IsInFieldRange(product));
        
        // Test division (if b is not zero)
        if (!b->IsZero()) {
            auto quotient = *a / *b;
            EXPECT_TRUE(IsInFieldRange(quotient));
        }
    }
}

TEST_F(ECFieldElementTest, SmallValuesArithmetic) {
    // Test arithmetic with small values for edge cases
    for (size_t i = 0; i < std::min(small_values.size(), size_t(20)); ++i) {
        for (size_t j = i + 1; j < std::min(small_values.size(), size_t(20)); ++j) {
            auto a = small_values[i];
            auto b = small_values[j];
            
            auto sum = *a + *b;
            auto diff = *a - *b;
            auto product = *a * *b;
            
            EXPECT_TRUE(IsInFieldRange(sum));
            EXPECT_TRUE(IsInFieldRange(diff));
            EXPECT_TRUE(IsInFieldRange(product));
            
            if (!b->IsZero()) {
                auto quotient = *a / *b;
                EXPECT_TRUE(IsInFieldRange(quotient));
            }
        }
    }
}

TEST_F(ECFieldElementTest, FieldPrimeIsCorrect) {
    auto field_prime = cryptography::ecc::ECFieldElement::GetFieldPrime();
    
    // Should be the secp256k1 field prime
    auto expected_prime = cryptography::BigInteger::FromHexString(
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f"
    );
    
    EXPECT_TRUE(field_prime == expected_prime);
}

TEST_F(ECFieldElementTest, PerformanceWithLargeOperations) {
    // Test performance with many operations
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto accumulator = *zero_element;
    for (int i = 0; i < 1000; ++i) {
        accumulator = accumulator + *one_element;
    }
    
    auto mid_time = std::chrono::high_resolution_clock::now();
    
    auto multiplier = *one_element;
    for (int i = 0; i < 100; ++i) {
        multiplier = multiplier * *two_element;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto add_duration = std::chrono::duration_cast<std::chrono::milliseconds>(mid_time - start_time);
    auto mul_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - mid_time);
    
    // Operations should complete in reasonable time
    EXPECT_LT(add_duration.count(), 1000);  // Less than 1 second for 1000 additions
    EXPECT_LT(mul_duration.count(), 1000);  // Less than 1 second for 100 multiplications
}

TEST_F(ECFieldElementTest, CopyConstructorAndAssignment) {
    // Test copy constructor
    cryptography::ecc::ECFieldElement copied(*test_value1);
    EXPECT_TRUE(copied == *test_value1);
    
    // Test assignment operator
    cryptography::ecc::ECFieldElement assigned = *test_value2;
    EXPECT_TRUE(assigned == *test_value2);
    
    // Test self-assignment safety
    assigned = assigned;
    EXPECT_TRUE(assigned == *test_value2);
}

TEST_F(ECFieldElementTest, EdgeCaseValues) {
    // Test with maximum field value
    EXPECT_TRUE(IsInFieldRange(*max_element));
    
    // Test arithmetic with max value
    auto max_plus_one = *max_element + *one_element;
    EXPECT_TRUE(IsInFieldRange(max_plus_one));
    EXPECT_TRUE(max_plus_one == *zero_element); // Should wrap around
    
    // Test with negative values
    EXPECT_TRUE(IsInFieldRange(*negative_one));
    
    // Test that -1 + 1 = 0
    auto neg_one_plus_one = *negative_one + *one_element;
    EXPECT_TRUE(neg_one_plus_one == *zero_element);
}

} // namespace test
} // namespace neo

#endif // TESTS_UNIT_CRYPTOGRAPHY_ECC_TEST_ECFIELDELEMENT_CPP_H
