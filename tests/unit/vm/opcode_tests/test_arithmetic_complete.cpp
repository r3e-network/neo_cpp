#include <gtest/gtest.h>
#include "neo/vm/execution_engine.h"
#include "neo/vm/script_builder.h"
#include "neo/vm/stack_item.h"
#include "neo/vm/opcode.h"
#include "neo/io/byte_vector.h"
#include <cmath>
#include <limits>

using namespace neo;
using namespace neo::vm;

class ArithmeticOpcodeTest : public ::testing::Test {
protected:
    std::unique_ptr<ExecutionEngine> engine;
    
    void SetUp() override {
        engine = std::make_unique<ExecutionEngine>();
    }
    
    void ExecuteScript(const ByteVector& script) {
        engine->LoadScript(script);
        engine->Execute();
    }
    
    void CheckResult(const StackItem& expected) {
        ASSERT_EQ(engine->State(), VMState::HALT);
        ASSERT_EQ(engine->ResultStack().size(), 1);
        EXPECT_EQ(engine->ResultStack()[0], expected);
    }
    
    void CheckFault() {
        EXPECT_EQ(engine->State(), VMState::FAULT);
    }
};

// ADD Tests
TEST_F(ArithmeticOpcodeTest, ADD_TwoPositiveIntegers) {
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::ADD);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(8));
}

TEST_F(ArithmeticOpcodeTest, ADD_PositiveAndNegative) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(-3);
    sb.EmitOpCode(OpCode::ADD);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(7));
}

TEST_F(ArithmeticOpcodeTest, ADD_LargeNumbers) {
    ScriptBuilder sb;
    sb.EmitPush(BigInteger("999999999999999999"));
    sb.EmitPush(BigInteger("1"));
    sb.EmitOpCode(OpCode::ADD);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(BigInteger("1000000000000000000")));
}

TEST_F(ArithmeticOpcodeTest, ADD_ByteArrays) {
    ScriptBuilder sb;
    sb.EmitPush(ByteVector{0x01, 0x02, 0x03}); // 197121 in little-endian
    sb.EmitPush(ByteVector{0x04, 0x05});       // 1284 in little-endian
    sb.EmitOpCode(OpCode::ADD);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(198405)); // 197121 + 1284
}

// SUB Tests
TEST_F(ArithmeticOpcodeTest, SUB_TwoPositiveIntegers) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::SUB);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(7));
}

TEST_F(ArithmeticOpcodeTest, SUB_ResultNegative) {
    ScriptBuilder sb;
    sb.EmitPush(3);
    sb.EmitPush(10);
    sb.EmitOpCode(OpCode::SUB);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(-7));
}

// MUL Tests
TEST_F(ArithmeticOpcodeTest, MUL_TwoPositiveIntegers) {
    ScriptBuilder sb;
    sb.EmitPush(6);
    sb.EmitPush(7);
    sb.EmitOpCode(OpCode::MUL);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(42));
}

TEST_F(ArithmeticOpcodeTest, MUL_ByZero) {
    ScriptBuilder sb;
    sb.EmitPush(100);
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::MUL);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(0));
}

TEST_F(ArithmeticOpcodeTest, MUL_NegativeNumbers) {
    ScriptBuilder sb;
    sb.EmitPush(-5);
    sb.EmitPush(-3);
    sb.EmitOpCode(OpCode::MUL);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(15));
}

// DIV Tests
TEST_F(ArithmeticOpcodeTest, DIV_ExactDivision) {
    ScriptBuilder sb;
    sb.EmitPush(20);
    sb.EmitPush(4);
    sb.EmitOpCode(OpCode::DIV);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(5));
}

TEST_F(ArithmeticOpcodeTest, DIV_IntegerDivision) {
    ScriptBuilder sb;
    sb.EmitPush(21);
    sb.EmitPush(4);
    sb.EmitOpCode(OpCode::DIV);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(5)); // Floor division
}

TEST_F(ArithmeticOpcodeTest, DIV_DivisionByZero) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::DIV);
    
    ExecuteScript(sb.ToByteArray());
    CheckFault(); // Should fault on division by zero
}

// MOD Tests
TEST_F(ArithmeticOpcodeTest, MOD_PositiveNumbers) {
    ScriptBuilder sb;
    sb.EmitPush(17);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::MOD);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(2));
}

TEST_F(ArithmeticOpcodeTest, MOD_NegativeDividend) {
    ScriptBuilder sb;
    sb.EmitPush(-17);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::MOD);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(-2));
}

TEST_F(ArithmeticOpcodeTest, MOD_ModuloByZero) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::MOD);
    
    ExecuteScript(sb.ToByteArray());
    CheckFault(); // Should fault on modulo by zero
}

// POW Tests
TEST_F(ArithmeticOpcodeTest, POW_PositiveExponent) {
    ScriptBuilder sb;
    sb.EmitPush(2);
    sb.EmitPush(8);
    sb.EmitOpCode(OpCode::POW);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(256));
}

TEST_F(ArithmeticOpcodeTest, POW_ZeroExponent) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::POW);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(1));
}

TEST_F(ArithmeticOpcodeTest, POW_OneBase) {
    ScriptBuilder sb;
    sb.EmitPush(1);
    sb.EmitPush(100);
    sb.EmitOpCode(OpCode::POW);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(1));
}

TEST_F(ArithmeticOpcodeTest, POW_NegativeExponent) {
    ScriptBuilder sb;
    sb.EmitPush(2);
    sb.EmitPush(-3);
    sb.EmitOpCode(OpCode::POW);
    
    ExecuteScript(sb.ToByteArray());
    CheckFault(); // Negative exponents should fault
}

// SQRT Tests
TEST_F(ArithmeticOpcodeTest, SQRT_PerfectSquare) {
    ScriptBuilder sb;
    sb.EmitPush(144);
    sb.EmitOpCode(OpCode::SQRT);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(12));
}

TEST_F(ArithmeticOpcodeTest, SQRT_NonPerfectSquare) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitOpCode(OpCode::SQRT);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(3)); // Floor of sqrt(10)
}

TEST_F(ArithmeticOpcodeTest, SQRT_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::SQRT);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(0));
}

TEST_F(ArithmeticOpcodeTest, SQRT_NegativeNumber) {
    ScriptBuilder sb;
    sb.EmitPush(-1);
    sb.EmitOpCode(OpCode::SQRT);
    
    ExecuteScript(sb.ToByteArray());
    CheckFault(); // Square root of negative should fault
}

// MODMUL Tests (a * b % m)
TEST_F(ArithmeticOpcodeTest, MODMUL_BasicOperation) {
    ScriptBuilder sb;
    sb.EmitPush(4);    // a
    sb.EmitPush(5);    // b
    sb.EmitPush(7);    // m
    sb.EmitOpCode(OpCode::MODMUL);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(6)); // (4 * 5) % 7 = 20 % 7 = 6
}

TEST_F(ArithmeticOpcodeTest, MODMUL_LargeNumbers) {
    ScriptBuilder sb;
    sb.EmitPush(BigInteger("999999999"));
    sb.EmitPush(BigInteger("999999999"));
    sb.EmitPush(BigInteger("1000000007"));
    sb.EmitOpCode(OpCode::MODMUL);
    
    ExecuteScript(sb.ToByteArray());
    // (999999999 * 999999999) % 1000000007
    CheckResult(StackItem::FromInteger(BigInteger("999999986")));
}

// MODPOW Tests (a^b % m)
TEST_F(ArithmeticOpcodeTest, MODPOW_BasicOperation) {
    ScriptBuilder sb;
    sb.EmitPush(3);    // base
    sb.EmitPush(4);    // exponent
    sb.EmitPush(5);    // modulus
    sb.EmitOpCode(OpCode::MODPOW);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(1)); // (3^4) % 5 = 81 % 5 = 1
}

TEST_F(ArithmeticOpcodeTest, MODPOW_ZeroExponent) {
    ScriptBuilder sb;
    sb.EmitPush(10);   // base
    sb.EmitPush(0);    // exponent
    sb.EmitPush(7);    // modulus
    sb.EmitOpCode(OpCode::MODPOW);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(1)); // (10^0) % 7 = 1 % 7 = 1
}

// ABS Tests
TEST_F(ArithmeticOpcodeTest, ABS_PositiveNumber) {
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitOpCode(OpCode::ABS);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(42));
}

TEST_F(ArithmeticOpcodeTest, ABS_NegativeNumber) {
    ScriptBuilder sb;
    sb.EmitPush(-42);
    sb.EmitOpCode(OpCode::ABS);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(42));
}

TEST_F(ArithmeticOpcodeTest, ABS_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::ABS);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(0));
}

// NEGATE Tests
TEST_F(ArithmeticOpcodeTest, NEGATE_PositiveNumber) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitOpCode(OpCode::NEGATE);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(-10));
}

TEST_F(ArithmeticOpcodeTest, NEGATE_NegativeNumber) {
    ScriptBuilder sb;
    sb.EmitPush(-10);
    sb.EmitOpCode(OpCode::NEGATE);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(10));
}

TEST_F(ArithmeticOpcodeTest, NEGATE_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::NEGATE);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(0));
}

// INC Tests
TEST_F(ArithmeticOpcodeTest, INC_PositiveNumber) {
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::INC);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(6));
}

TEST_F(ArithmeticOpcodeTest, INC_NegativeNumber) {
    ScriptBuilder sb;
    sb.EmitPush(-1);
    sb.EmitOpCode(OpCode::INC);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(0));
}

// DEC Tests
TEST_F(ArithmeticOpcodeTest, DEC_PositiveNumber) {
    ScriptBuilder sb;
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::DEC);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(4));
}

TEST_F(ArithmeticOpcodeTest, DEC_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::DEC);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(-1));
}

// SIGN Tests
TEST_F(ArithmeticOpcodeTest, SIGN_PositiveNumber) {
    ScriptBuilder sb;
    sb.EmitPush(42);
    sb.EmitOpCode(OpCode::SIGN);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(1));
}

TEST_F(ArithmeticOpcodeTest, SIGN_NegativeNumber) {
    ScriptBuilder sb;
    sb.EmitPush(-42);
    sb.EmitOpCode(OpCode::SIGN);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(-1));
}

TEST_F(ArithmeticOpcodeTest, SIGN_Zero) {
    ScriptBuilder sb;
    sb.EmitPush(0);
    sb.EmitOpCode(OpCode::SIGN);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(0));
}

// MIN/MAX Tests
TEST_F(ArithmeticOpcodeTest, MIN_FirstSmaller) {
    ScriptBuilder sb;
    sb.EmitPush(3);
    sb.EmitPush(7);
    sb.EmitOpCode(OpCode::MIN);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(3));
}

TEST_F(ArithmeticOpcodeTest, MIN_SecondSmaller) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::MIN);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(5));
}

TEST_F(ArithmeticOpcodeTest, MAX_FirstLarger) {
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::MAX);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(10));
}

TEST_F(ArithmeticOpcodeTest, MAX_SecondLarger) {
    ScriptBuilder sb;
    sb.EmitPush(3);
    sb.EmitPush(7);
    sb.EmitOpCode(OpCode::MAX);
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(7));
}

// Edge case tests
TEST_F(ArithmeticOpcodeTest, ArithmeticOverflow) {
    ScriptBuilder sb;
    sb.EmitPush(BigInteger::Pow(2, 255) - 1); // Max positive value
    sb.EmitPush(1);
    sb.EmitOpCode(OpCode::ADD);
    
    ExecuteScript(sb.ToByteArray());
    // Should handle overflow correctly
    CheckResult(StackItem::FromInteger(BigInteger::Pow(2, 255)));
}

TEST_F(ArithmeticOpcodeTest, ComplexArithmeticExpression) {
    // Calculate: ((10 + 5) * 3 - 20) / 5 = (15 * 3 - 20) / 5 = (45 - 20) / 5 = 25 / 5 = 5
    ScriptBuilder sb;
    sb.EmitPush(10);
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::ADD);    // 15
    sb.EmitPush(3);
    sb.EmitOpCode(OpCode::MUL);    // 45
    sb.EmitPush(20);
    sb.EmitOpCode(OpCode::SUB);    // 25
    sb.EmitPush(5);
    sb.EmitOpCode(OpCode::DIV);    // 5
    
    ExecuteScript(sb.ToByteArray());
    CheckResult(StackItem::FromInteger(5));
}