#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_control.h>
#include <neo/vm/jump_table_control_jump.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>

namespace neo::vm
{
    // JumpTable delegates to JumpTableControlJump
    void JumpTable::JMP(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMP(engine, instruction);
    }

    void JumpTable::JMP_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMP_L(engine, instruction);
    }

    void JumpTable::JMPIF(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPIF(engine, instruction);
    }

    void JumpTable::JMPIF_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPIF_L(engine, instruction);
    }

    void JumpTable::JMPIFNOT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPIFNOT(engine, instruction);
    }

    void JumpTable::JMPIFNOT_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPIFNOT_L(engine, instruction);
    }

    void JumpTable::JMPEQ(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPEQ(engine, instruction);
    }

    void JumpTable::JMPEQ_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPEQ_L(engine, instruction);
    }

    void JumpTable::JMPNE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPNE(engine, instruction);
    }

    void JumpTable::JMPNE_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPNE_L(engine, instruction);
    }

    void JumpTable::JMPGT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPGT(engine, instruction);
    }

    void JumpTable::JMPGT_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPGT_L(engine, instruction);
    }

    void JumpTable::JMPGE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPGE(engine, instruction);
    }

    void JumpTable::JMPGE_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPGE_L(engine, instruction);
    }

    void JumpTable::JMPLT(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPLT(engine, instruction);
    }

    void JumpTable::JMPLT_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPLT_L(engine, instruction);
    }

    void JumpTable::JMPLE(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPLE(engine, instruction);
    }

    void JumpTable::JMPLE_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::JMPLE_L(engine, instruction);
    }

    void JumpTable::CALL(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::CALL(engine, instruction);
    }

    void JumpTable::CALL_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::CALL_L(engine, instruction);
    }

    void JumpTable::CALLA(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        auto item = engine.Pop();
        if (item->GetType() != StackItemType::Pointer)
            throw InvalidOperationException("Item is not a pointer");

        auto pointerItem = std::dynamic_pointer_cast<PointerItem>(item);
        engine.ExecuteCall(pointerItem->GetPosition());
    }

    void JumpTable::RET(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.ExecuteRet();
    }

    void JumpTable::SYSCALL(ExecutionEngine& engine, const Instruction& instruction)
    {
        JumpTableControlJump::SYSCALL(engine, instruction);
    }

    void JumpTable::ExecuteJumpOffset(ExecutionEngine& engine, int32_t offset)
    {
        JumpTableControlJump::ExecuteJumpOffset(engine, offset);
    }

    // JumpTableControlJump implementations
    void JumpTableControlJump::JMP(ExecutionEngine& engine, const Instruction& instruction)
    {
        ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMP_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPIF(ExecutionEngine& engine, const Instruction& instruction)
    {
        if (engine.Pop()->GetBoolean())
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPIF_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        if (engine.Pop()->GetBoolean())
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPIFNOT(ExecutionEngine& engine, const Instruction& instruction)
    {
        if (!engine.Pop()->GetBoolean())
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPIFNOT_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        if (!engine.Pop()->GetBoolean())
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPEQ(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 == x2)
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPEQ_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 == x2)
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPNE(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 != x2)
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPNE_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 != x2)
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPGT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 > x2)
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPGT_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 > x2)
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPGE(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 >= x2)
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPGE_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 >= x2)
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPLT(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 < x2)
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPLT_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 < x2)
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::JMPLE(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 <= x2)
            ExecuteJumpOffset(engine, instruction.TokenI8());
    }

    void JumpTableControlJump::JMPLE_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto x2 = engine.Pop()->GetInteger();
        auto x1 = engine.Pop()->GetInteger();
        if (x1 <= x2)
            ExecuteJumpOffset(engine, instruction.TokenI32());
    }

    void JumpTableControlJump::CALL(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto& context = engine.GetCurrentContext();
        int32_t offset = instruction.TokenI8();
        int32_t position = context.GetInstructionPointer() + offset;
        engine.ExecuteCall(position);
    }

    void JumpTableControlJump::CALL_L(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto& context = engine.GetCurrentContext();
        int32_t offset = instruction.TokenI32();
        int32_t position = context.GetInstructionPointer() + offset;
        engine.ExecuteCall(position);
    }

    void JumpTableControlJump::SYSCALL(ExecutionEngine& engine, const Instruction& instruction)
    {
        uint32_t hash = instruction.TokenU32();
        engine.ExecuteSysCall(hash);
    }

    void JumpTableControlJump::ExecuteJumpOffset(ExecutionEngine& engine, int32_t offset)
    {
        auto& context = engine.GetCurrentContext();
        int32_t position = context.GetInstructionPointer() + offset;
        engine.ExecuteJump(position);
    }
}
