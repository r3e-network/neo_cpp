#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_slot.h>
#include <neo/vm/jump_table_slot_static.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>

namespace neo::vm
{
    // JumpTable delegates to JumpTableSlotStatic
    void JumpTable::INITSSLOT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::INITSSLOT(engine, instruction);
    }

    void JumpTable::LDSFLD0(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD0(engine, instruction);
    }

    void JumpTable::LDSFLD1(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD1(engine, instruction);
    }

    void JumpTable::LDSFLD2(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD2(engine, instruction);
    }

    void JumpTable::LDSFLD3(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD3(engine, instruction);
    }

    void JumpTable::LDSFLD4(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD4(engine, instruction);
    }

    void JumpTable::LDSFLD5(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD5(engine, instruction);
    }

    void JumpTable::LDSFLD6(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD6(engine, instruction);
    }

    void JumpTable::LDSFLD(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::LDSFLD(engine, instruction);
    }

    void JumpTable::STSFLD0(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD0(engine, instruction);
    }

    void JumpTable::STSFLD1(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD1(engine, instruction);
    }

    void JumpTable::STSFLD2(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD2(engine, instruction);
    }

    void JumpTable::STSFLD3(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD3(engine, instruction);
    }

    void JumpTable::STSFLD4(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD4(engine, instruction);
    }

    void JumpTable::STSFLD5(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD5(engine, instruction);
    }

    void JumpTable::STSFLD6(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD6(engine, instruction);
    }

    void JumpTable::STSFLD(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableSlotStatic::STSFLD(engine, instruction);
    }

    // JumpTableSlotStatic implementations
    void JumpTableSlotStatic::INITSSLOT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto count = instruction.TokenU8();
        auto& context = engine.GetCurrentContext();
        context.InitializeStaticFields(count);
    }

    void JumpTableSlotStatic::LDSFLD0(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(0));
    }

    void JumpTableSlotStatic::LDSFLD1(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(1));
    }

    void JumpTableSlotStatic::LDSFLD2(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(2));
    }

    void JumpTableSlotStatic::LDSFLD3(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(3));
    }

    void JumpTableSlotStatic::LDSFLD4(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(4));
    }

    void JumpTableSlotStatic::LDSFLD5(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(5));
    }

    void JumpTableSlotStatic::LDSFLD6(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(6));
    }

    void JumpTableSlotStatic::LDSFLD(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto index = instruction.TokenU8();
        auto& context = engine.GetCurrentContext();
        engine.Push(context.GetStaticField(index));
    }

    void JumpTableSlotStatic::STSFLD0(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(0, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD1(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(1, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD2(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(2, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD3(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(3, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD4(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(4, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD5(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(5, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD6(ExecutionEngine& engine, const Instruction& instruction)
    {
        (void)instruction;
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(6, engine.Pop());
    }

    void JumpTableSlotStatic::STSFLD(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto index = instruction.TokenU8();
        auto& context = engine.GetCurrentContext();
        context.SetStaticField(index, engine.Pop());
    }
}
