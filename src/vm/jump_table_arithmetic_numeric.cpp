/**
 * @file jump_table_arithmetic_numeric.cpp
 * @brief Jump Table Arithmetic Numeric
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_arithmetic.h>
#include <neo/vm/jump_table_arithmetic_numeric.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/stack_item.h>

#include <cmath>

namespace neo::vm
{
// JumpTable delegates to JumpTableArithmeticNumeric
void JumpTable::ADD(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::ADD(engine, instruction);
}

void JumpTable::SUB(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::SUB(engine, instruction);
}

void JumpTable::MUL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::MUL(engine, instruction);
}

void JumpTable::DIV(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::DIV(engine, instruction);
}

void JumpTable::MOD(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::MOD(engine, instruction);
}

void JumpTable::POW(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::POW(engine, instruction);
}

void JumpTable::SQRT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::SQRT(engine, instruction);
}

void JumpTable::SHL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::SHL(engine, instruction);
}

void JumpTable::SHR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::SHR(engine, instruction);
}

void JumpTable::NOT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::NOT(engine, instruction);
}

void JumpTable::BOOLAND(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::BOOLAND(engine, instruction);
}

void JumpTable::BOOLOR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::BOOLOR(engine, instruction);
}

void JumpTable::NUMEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::NUMEQUAL(engine, instruction);
}

void JumpTable::NUMNOTEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::NUMNOTEQUAL(engine, instruction);
}

void JumpTable::LT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::LT(engine, instruction);
}

void JumpTable::GT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::GT(engine, instruction);
}

void JumpTable::LE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::LE(engine, instruction);
}

void JumpTable::GE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::GE(engine, instruction);
}

void JumpTable::MIN(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::MIN(engine, instruction);
}

void JumpTable::MAX(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::MAX(engine, instruction);
}

void JumpTable::WITHIN(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::WITHIN(engine, instruction);
}

void JumpTable::SIGN(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::SIGN(engine, instruction);
}

void JumpTable::ABS(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::ABS(engine, instruction);
}

void JumpTable::NEGATE(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::NEGATE(engine, instruction);
}

void JumpTable::INC(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::INC(engine, instruction);
}

void JumpTable::DEC(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::DEC(engine, instruction);
}

void JumpTable::MODMUL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::MODMUL(engine, instruction);
}

void JumpTable::MODPOW(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::MODPOW(engine, instruction);
}

void JumpTable::NZ(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticNumeric::NZ(engine, instruction);
}

// JumpTableArithmeticNumeric implementations
void JumpTableArithmeticNumeric::ADD(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 + x2));
}

void JumpTableArithmeticNumeric::SUB(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 - x2));
}

void JumpTableArithmeticNumeric::MUL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 * x2));
}

void JumpTableArithmeticNumeric::DIV(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    if (x2 == 0) throw DivideByZeroException();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 / x2));
}

void JumpTableArithmeticNumeric::MOD(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    if (x2 == 0) throw DivideByZeroException();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 % x2));
}

void JumpTableArithmeticNumeric::POW(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto exponent = engine.Pop()->GetInteger();
    auto value = engine.Pop()->GetInteger();

    if (exponent < 0) throw InvalidOperationException("Exponent cannot be negative");

    if (exponent == 0)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(1)));
        return;
    }

    if (value == 0)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(0)));
        return;
    }

    // Use binary exponentiation for large exponents
    int64_t result = 1;
    int64_t base = value;

    while (exponent > 0)
    {
        if (exponent % 2 == 1) result *= base;

        base *= base;
        exponent /= 2;
    }

    engine.Push(StackItem::Create(result));
}

void JumpTableArithmeticNumeric::SQRT(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    if (x < 0) throw InvalidOperationException("Cannot calculate square root of negative number");

    engine.Push(StackItem::Create(static_cast<int64_t>(std::sqrt(static_cast<double>(x)))));
}

void JumpTableArithmeticNumeric::SHL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto shift = engine.Pop()->GetInteger();
    auto value = engine.Pop()->GetInteger();

    if (shift < 0) throw InvalidOperationException("Shift value cannot be negative");

    if (shift > 63)
        engine.Push(StackItem::Create(static_cast<int64_t>(0)));
    else
        engine.Push(StackItem::Create(value << shift));
}

void JumpTableArithmeticNumeric::SHR(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto shift = engine.Pop()->GetInteger();
    auto value = engine.Pop()->GetInteger();

    if (shift < 0) throw InvalidOperationException("Shift value cannot be negative");

    if (shift > 63)
        engine.Push(StackItem::Create(value >= 0 ? static_cast<int64_t>(0) : static_cast<int64_t>(-1)));
    else
        engine.Push(StackItem::Create(value >> shift));
}

void JumpTableArithmeticNumeric::NOT(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetBoolean();
    engine.Push(StackItem::Create(!x));
}

void JumpTableArithmeticNumeric::BOOLAND(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetBoolean();
    auto x1 = engine.Pop()->GetBoolean();
    engine.Push(StackItem::Create(x1 && x2));
}

void JumpTableArithmeticNumeric::BOOLOR(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetBoolean();
    auto x1 = engine.Pop()->GetBoolean();
    engine.Push(StackItem::Create(x1 || x2));
}

void JumpTableArithmeticNumeric::NUMEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 == x2));
}

void JumpTableArithmeticNumeric::NUMNOTEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 != x2));
}

void JumpTableArithmeticNumeric::LT(ExecutionEngine& engine, const Instruction& instruction)
{
    auto item2 = engine.Pop();
    auto item1 = engine.Pop();

    auto isNumericComparable = [](const std::shared_ptr<StackItem>& item) {
        auto type = item->GetType();
        return type == StackItemType::Integer || type == StackItemType::Boolean || item->IsNull();
    };

    if (!isNumericComparable(item1) || !isNumericComparable(item2))
    {
        throw std::runtime_error("Invalid comparison operation");
    }

    // Handle null values - treat null as integer 0
    int64_t x2 = item2->IsNull() ? 0 : item2->GetInteger();
    int64_t x1 = item1->IsNull() ? 0 : item1->GetInteger();

    engine.Push(StackItem::Create(x1 < x2));
}

void JumpTableArithmeticNumeric::GT(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto item2 = engine.Pop();
    auto item1 = engine.Pop();

    auto isNumericComparable = [](const std::shared_ptr<StackItem>& item) {
        auto type = item->GetType();
        return type == StackItemType::Integer || type == StackItemType::Boolean || item->IsNull();
    };

    if (!isNumericComparable(item1) || !isNumericComparable(item2))
    {
        throw std::runtime_error("Invalid comparison operation");
    }

    // Handle null values - treat null as integer 0
    int64_t x2 = item2->IsNull() ? 0 : item2->GetInteger();
    int64_t x1 = item1->IsNull() ? 0 : item1->GetInteger();

    engine.Push(StackItem::Create(x1 > x2));
}

void JumpTableArithmeticNumeric::LE(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto item2 = engine.Pop();
    auto item1 = engine.Pop();

    auto isNumericComparable = [](const std::shared_ptr<StackItem>& item) {
        auto type = item->GetType();
        return type == StackItemType::Integer || type == StackItemType::Boolean || item->IsNull();
    };

    if (!isNumericComparable(item1) || !isNumericComparable(item2))
    {
        throw std::runtime_error("Invalid comparison operation");
    }

    // Handle null values - treat null as integer 0
    int64_t x2 = item2->IsNull() ? 0 : item2->GetInteger();
    int64_t x1 = item1->IsNull() ? 0 : item1->GetInteger();

    engine.Push(StackItem::Create(x1 <= x2));
}

void JumpTableArithmeticNumeric::GE(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto item2 = engine.Pop();
    auto item1 = engine.Pop();

    auto isNumericComparable = [](const std::shared_ptr<StackItem>& item) {
        auto type = item->GetType();
        return type == StackItemType::Integer || type == StackItemType::Boolean || item->IsNull();
    };

    if (!isNumericComparable(item1) || !isNumericComparable(item2))
    {
        throw std::runtime_error("Invalid comparison operation");
    }

    // Handle null values - treat null as integer 0
    int64_t x2 = item2->IsNull() ? 0 : item2->GetInteger();
    int64_t x1 = item1->IsNull() ? 0 : item1->GetInteger();

    engine.Push(StackItem::Create(x1 >= x2));
}

void JumpTableArithmeticNumeric::MIN(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(std::min(x1, x2)));
}

void JumpTableArithmeticNumeric::MAX(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(std::max(x1, x2)));
}

void JumpTableArithmeticNumeric::WITHIN(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 3)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto b = engine.Pop()->GetInteger();
    auto a = engine.Pop()->GetInteger();
    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(a <= x && x < b));
}

void JumpTableArithmeticNumeric::SIGN(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    int64_t sign = (x > 0) ? 1 : ((x < 0) ? -1 : 0);
    engine.Push(StackItem::Create(sign));
}

void JumpTableArithmeticNumeric::ABS(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(std::abs(x)));
}

void JumpTableArithmeticNumeric::NEGATE(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(-x));
}

void JumpTableArithmeticNumeric::INC(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x + 1));
}

void JumpTableArithmeticNumeric::DEC(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x - 1));
}

void JumpTableArithmeticNumeric::MODMUL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 3)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto modulus = engine.Pop()->GetInteger();
    if (modulus <= 0) throw InvalidOperationException("Modulus must be positive");

    auto b = engine.Pop()->GetInteger();
    auto a = engine.Pop()->GetInteger();

    // Calculate (a * b) % modulus without overflow
    int64_t result = 0;
    a = a % modulus;
    b = b % modulus;

    while (b > 0)
    {
        if (b % 2 == 1) result = (result + a) % modulus;

        a = (a * 2) % modulus;
        b /= 2;
    }

    engine.Push(StackItem::Create(result));
}

void JumpTableArithmeticNumeric::MODPOW(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 3)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto modulus = engine.Pop()->GetInteger();
    if (modulus <= 0) throw InvalidOperationException("Modulus must be positive");

    auto exponent = engine.Pop()->GetInteger();
    if (exponent < 0) throw InvalidOperationException("Exponent cannot be negative");

    auto value = engine.Pop()->GetInteger();

    // Calculate (value ^ exponent) % modulus using binary exponentiation
    int64_t result = 1;
    value = value % modulus;

    while (exponent > 0)
    {
        if (exponent % 2 == 1) result = (result * value) % modulus;

        value = (value * value) % modulus;
        exponent /= 2;
    }

    engine.Push(StackItem::Create(result));
}

void JumpTableArithmeticNumeric::NZ(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x != 0));
}
}  // namespace neo::vm
