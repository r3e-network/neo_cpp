#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_slot.h>
#include <neo/vm/jump_table_slot_argument.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// JumpTable delegates to JumpTableSlotArgument
void JumpTable::LDARG0(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG0(engine, instruction);
}

void JumpTable::LDARG1(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG1(engine, instruction);
}

void JumpTable::LDARG2(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG2(engine, instruction);
}

void JumpTable::LDARG3(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG3(engine, instruction);
}

void JumpTable::LDARG4(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG4(engine, instruction);
}

void JumpTable::LDARG5(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG5(engine, instruction);
}

void JumpTable::LDARG6(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG6(engine, instruction);
}

void JumpTable::LDARG(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::LDARG(engine, instruction);
}

void JumpTable::STARG0(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG0(engine, instruction);
}

void JumpTable::STARG1(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG1(engine, instruction);
}

void JumpTable::STARG2(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG2(engine, instruction);
}

void JumpTable::STARG3(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG3(engine, instruction);
}

void JumpTable::STARG4(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG4(engine, instruction);
}

void JumpTable::STARG5(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG5(engine, instruction);
}

void JumpTable::STARG6(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG6(engine, instruction);
}

void JumpTable::STARG(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotArgument::STARG(engine, instruction);
}

// JumpTableSlotArgument implementations
void JumpTableSlotArgument::LDARG0(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(0));
}

void JumpTableSlotArgument::LDARG1(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(1));
}

void JumpTableSlotArgument::LDARG2(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(2));
}

void JumpTableSlotArgument::LDARG3(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(3));
}

void JumpTableSlotArgument::LDARG4(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(4));
}

void JumpTableSlotArgument::LDARG5(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(5));
}

void JumpTableSlotArgument::LDARG6(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(6));
}

void JumpTableSlotArgument::LDARG(ExecutionEngine& engine, const Instruction& instruction)
{
    auto index = instruction.TokenU8();
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadArgument(index));
}

void JumpTableSlotArgument::STARG0(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(0, engine.Pop());
}

void JumpTableSlotArgument::STARG1(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(1, engine.Pop());
}

void JumpTableSlotArgument::STARG2(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(2, engine.Pop());
}

void JumpTableSlotArgument::STARG3(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(3, engine.Pop());
}

void JumpTableSlotArgument::STARG4(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(4, engine.Pop());
}

void JumpTableSlotArgument::STARG5(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(5, engine.Pop());
}

void JumpTableSlotArgument::STARG6(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(6, engine.Pop());
}

void JumpTableSlotArgument::STARG(ExecutionEngine& engine, const Instruction& instruction)
{
    auto index = instruction.TokenU8();
    auto& context = engine.GetCurrentContext();
    context.StoreArgument(index, engine.Pop());
}
}  // namespace neo::vm
