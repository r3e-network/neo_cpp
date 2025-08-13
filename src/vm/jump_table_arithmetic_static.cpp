/**
 * @file jump_table_arithmetic_static.cpp
 * @brief Jump Table Arithmetic Static
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table_arithmetic.h>
#include <neo/vm/stack_item.h>

#include <cmath>

namespace neo::vm
{
// Arithmetic operations

void JumpTableArithmetic::ADD(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::ADD(engine, instruction);
}

void JumpTableArithmetic::SUB(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::SUB(engine, instruction);
}

void JumpTableArithmetic::MUL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::MUL(engine, instruction);
}

void JumpTableArithmetic::DIV(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::DIV(engine, instruction);
}

void JumpTableArithmetic::MOD(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::MOD(engine, instruction);
}

void JumpTableArithmetic::POW(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::POW(engine, instruction);
}

void JumpTableArithmetic::SQRT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::SQRT(engine, instruction);
}

void JumpTableArithmetic::SHL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::SHL(engine, instruction);
}

void JumpTableArithmetic::SHR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::SHR(engine, instruction);
}

void JumpTableArithmetic::NOT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::NOT(engine, instruction);
}

void JumpTableArithmetic::BOOLAND(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::BOOLAND(engine, instruction);
}

void JumpTableArithmetic::BOOLOR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::BOOLOR(engine, instruction);
}

void JumpTableArithmetic::NUMEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::NUMEQUAL(engine, instruction);
}

void JumpTableArithmetic::NUMNOTEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::NUMNOTEQUAL(engine, instruction);
}

void JumpTableArithmetic::LT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::LT(engine, instruction);
}

void JumpTableArithmetic::GT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::GT(engine, instruction);
}

void JumpTableArithmetic::LE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::LE(engine, instruction);
}

void JumpTableArithmetic::GE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::GE(engine, instruction);
}

void JumpTableArithmetic::MIN(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::MIN(engine, instruction);
}

void JumpTableArithmetic::MAX(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::MAX(engine, instruction);
}

void JumpTableArithmetic::WITHIN(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::WITHIN(engine, instruction);
}

void JumpTableArithmetic::SIGN(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::SIGN(engine, instruction);
}

void JumpTableArithmetic::ABS(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::ABS(engine, instruction);
}

void JumpTableArithmetic::NEGATE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::NEGATE(engine, instruction);
}

void JumpTableArithmetic::INC(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::INC(engine, instruction);
}

void JumpTableArithmetic::DEC(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::DEC(engine, instruction);
}

void JumpTableArithmetic::INVERT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::INVERT(engine, instruction);
}

void JumpTableArithmetic::AND(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::AND(engine, instruction);
}

void JumpTableArithmetic::OR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::OR(engine, instruction);
}

void JumpTableArithmetic::XOR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::XOR(engine, instruction);
}

void JumpTableArithmetic::EQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::EQUAL(engine, instruction);
}

void JumpTableArithmetic::NOTEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::NOTEQUAL(engine, instruction);
}

void JumpTableArithmetic::MODMUL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::MODMUL(engine, instruction);
}

void JumpTableArithmetic::MODPOW(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::MODPOW(engine, instruction);
}

void JumpTableArithmetic::NZ(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTable::NZ(engine, instruction);
}
}  // namespace neo::vm
