#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_arithmetic.h>
#include <neo/vm/jump_table_arithmetic_bitwise.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// JumpTable delegates to JumpTableArithmeticBitwise
void JumpTable::INVERT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticBitwise::INVERT(engine, instruction);
}

void JumpTable::AND(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticBitwise::AND(engine, instruction);
}

void JumpTable::OR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticBitwise::OR(engine, instruction);
}

void JumpTable::XOR(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticBitwise::XOR(engine, instruction);
}

void JumpTable::EQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticBitwise::EQUAL(engine, instruction);
}

void JumpTable::NOTEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableArithmeticBitwise::NOTEQUAL(engine, instruction);
}

// JumpTableArithmeticBitwise implementations
void JumpTableArithmeticBitwise::INVERT(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 1)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(~x));
}

void JumpTableArithmeticBitwise::AND(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 & x2));
}

void JumpTableArithmeticBitwise::OR(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 | x2));
}

void JumpTableArithmeticBitwise::XOR(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop()->GetInteger();
    auto x1 = engine.Pop()->GetInteger();
    engine.Push(StackItem::Create(x1 ^ x2));
}

void JumpTableArithmeticBitwise::EQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop();
    auto x1 = engine.Pop();
    engine.Push(StackItem::Create(x1->Equals(*x2)));
}

void JumpTableArithmeticBitwise::NOTEQUAL(ExecutionEngine& engine, const Instruction& instruction)
{
    // Check stack size before popping
    if (engine.GetCurrentContext().GetStackSize() < 2)
    {
        throw std::runtime_error("Stack underflow");
    }

    auto x2 = engine.Pop();
    auto x1 = engine.Pop();
    engine.Push(StackItem::Create(!x1->Equals(*x2)));
}
}  // namespace neo::vm
