#include <neo/vm/compound_items.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_slot.h>
#include <neo/vm/jump_table_slot_local.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/special_items.h>
#include <neo/vm/stack_item.h>

namespace neo::vm
{
// JumpTable delegates to JumpTableSlotLocal
void JumpTable::INITSLOT(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::INITSLOT(engine, instruction);
}

void JumpTable::LDLOC0(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC0(engine, instruction);
}

void JumpTable::LDLOC1(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC1(engine, instruction);
}

void JumpTable::LDLOC2(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC2(engine, instruction);
}

void JumpTable::LDLOC3(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC3(engine, instruction);
}

void JumpTable::LDLOC4(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC4(engine, instruction);
}

void JumpTable::LDLOC5(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC5(engine, instruction);
}

void JumpTable::LDLOC6(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC6(engine, instruction);
}

void JumpTable::LDLOC(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::LDLOC(engine, instruction);
}

void JumpTable::STLOC0(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC0(engine, instruction);
}

void JumpTable::STLOC1(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC1(engine, instruction);
}

void JumpTable::STLOC2(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC2(engine, instruction);
}

void JumpTable::STLOC3(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC3(engine, instruction);
}

void JumpTable::STLOC4(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC4(engine, instruction);
}

void JumpTable::STLOC5(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC5(engine, instruction);
}

void JumpTable::STLOC6(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC6(engine, instruction);
}

void JumpTable::STLOC(ExecutionEngine& engine, const Instruction& instruction)
{
    JumpTableSlotLocal::STLOC(engine, instruction);
}

// JumpTableSlotLocal implementations
void JumpTableSlotLocal::INITSLOT(ExecutionEngine& engine, const Instruction& instruction)
{
    auto localCount = instruction.TokenU8();
    auto paramCount = instruction.TokenU8_1();
    auto& context = engine.GetCurrentContext();

    // Initialize local variables
    std::vector<std::shared_ptr<StackItem>> localVariables;
    for (uint8_t i = 0; i < localCount; i++)
    {
        localVariables.push_back(StackItem::Null());
    }
    context.SetLocalVariables(localVariables);

    // Initialize arguments
    std::vector<std::shared_ptr<StackItem>> arguments;
    for (uint8_t i = 0; i < paramCount; i++)
    {
        arguments.push_back(engine.Pop());
    }
    std::reverse(arguments.begin(), arguments.end());
    context.SetArguments(arguments);
}

void JumpTableSlotLocal::LDLOC0(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(0));
}

void JumpTableSlotLocal::LDLOC1(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(1));
}

void JumpTableSlotLocal::LDLOC2(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(2));
}

void JumpTableSlotLocal::LDLOC3(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(3));
}

void JumpTableSlotLocal::LDLOC4(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(4));
}

void JumpTableSlotLocal::LDLOC5(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(5));
}

void JumpTableSlotLocal::LDLOC6(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(6));
}

void JumpTableSlotLocal::LDLOC(ExecutionEngine& engine, const Instruction& instruction)
{
    auto index = instruction.TokenU8();
    auto& context = engine.GetCurrentContext();
    engine.Push(context.LoadLocalVariable(index));
}

void JumpTableSlotLocal::STLOC0(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(0, engine.Pop());
}

void JumpTableSlotLocal::STLOC1(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(1, engine.Pop());
}

void JumpTableSlotLocal::STLOC2(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(2, engine.Pop());
}

void JumpTableSlotLocal::STLOC3(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(3, engine.Pop());
}

void JumpTableSlotLocal::STLOC4(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(4, engine.Pop());
}

void JumpTableSlotLocal::STLOC5(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(5, engine.Pop());
}

void JumpTableSlotLocal::STLOC6(ExecutionEngine& engine, const Instruction& /* instruction */)
{
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(6, engine.Pop());
}

void JumpTableSlotLocal::STLOC(ExecutionEngine& engine, const Instruction& instruction)
{
    auto index = instruction.TokenU8();
    auto& context = engine.GetCurrentContext();
    context.StoreLocalVariable(index, engine.Pop());
}
}  // namespace neo::vm
