/**
 * @file jump_table.cpp
 * @brief Jump Table
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/vm/exceptions.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/jump_table.h>
#include <neo/vm/jump_table_arithmetic.h>
#include <neo/vm/jump_table_compound.h>
#include <neo/vm/jump_table_constants.h>
#include <neo/vm/jump_table_control_exception.h>
#include <neo/vm/jump_table_control_jump.h>
#include <neo/vm/jump_table_exception.h>
#include <neo/vm/jump_table_extension.h>
#include <neo/vm/jump_table_slot.h>
#include <neo/vm/jump_table_splice.h>
#include <neo/vm/jump_table_stack.h>
#include <neo/vm/jump_table_type.h>
#include <neo/vm/script.h>
#include <neo/vm/stack_item.h>

#include <stdexcept>

namespace neo::vm
{
JumpTable::JumpTable()
{
    // Initialize all handlers to InvalidOpcode
    for (auto& handler : handlers_)
    {
        handler = [this](ExecutionEngine& engine, const Instruction& instruction)
        { InvalidOpcode(engine, instruction); };
    }

    // Constants
    SetHandler(OpCode::PUSHINT8, JumpTableConstants::PUSHINT8);
    SetHandler(OpCode::PUSHINT16, JumpTableConstants::PUSHINT16);
    SetHandler(OpCode::PUSHINT32, JumpTableConstants::PUSHINT32);
    SetHandler(OpCode::PUSHINT64, JumpTableConstants::PUSHINT64);
    SetHandler(OpCode::PUSHINT128, JumpTableConstants::PUSHINT128);
    SetHandler(OpCode::PUSHINT256, JumpTableConstants::PUSHINT256);
    SetHandler(OpCode::PUSHA, JumpTableConstants::PUSHA);
    SetHandler(OpCode::PUSHNULL, JumpTableConstants::PUSHNULL);
    SetHandler(OpCode::PUSHDATA1, JumpTableConstants::PUSHDATA1);
    SetHandler(OpCode::PUSHDATA2, JumpTableConstants::PUSHDATA2);
    SetHandler(OpCode::PUSHDATA4, JumpTableConstants::PUSHDATA4);
    SetHandler(OpCode::PUSHM1, JumpTableConstants::PUSHM1);
    SetHandler(OpCode::PUSH0, JumpTableConstants::PUSH0);
    SetHandler(OpCode::PUSH1, JumpTableConstants::PUSH1);
    SetHandler(OpCode::PUSH2, JumpTableConstants::PUSH2);
    SetHandler(OpCode::PUSH3, JumpTableConstants::PUSH3);
    SetHandler(OpCode::PUSH4, JumpTableConstants::PUSH4);
    SetHandler(OpCode::PUSH5, JumpTableConstants::PUSH5);
    SetHandler(OpCode::PUSH6, JumpTableConstants::PUSH6);
    SetHandler(OpCode::PUSH7, JumpTableConstants::PUSH7);
    SetHandler(OpCode::PUSH8, JumpTableConstants::PUSH8);
    SetHandler(OpCode::PUSH9, JumpTableConstants::PUSH9);
    SetHandler(OpCode::PUSH10, JumpTableConstants::PUSH10);
    SetHandler(OpCode::PUSH11, JumpTableConstants::PUSH11);
    SetHandler(OpCode::PUSH12, JumpTableConstants::PUSH12);
    SetHandler(OpCode::PUSH13, JumpTableConstants::PUSH13);
    SetHandler(OpCode::PUSH14, JumpTableConstants::PUSH14);
    SetHandler(OpCode::PUSH15, JumpTableConstants::PUSH15);
    SetHandler(OpCode::PUSH16, JumpTableConstants::PUSH16);

    // Flow control - Jump operations
    SetHandler(OpCode::JMP, JumpTableControlJump::JMP);
    SetHandler(OpCode::JMP_L, JumpTableControlJump::JMP_L);
    SetHandler(OpCode::JMPIF, JumpTableControlJump::JMPIF);
    SetHandler(OpCode::JMPIF_L, JumpTableControlJump::JMPIF_L);
    SetHandler(OpCode::JMPIFNOT, JumpTableControlJump::JMPIFNOT);
    SetHandler(OpCode::JMPIFNOT_L, JumpTableControlJump::JMPIFNOT_L);
    SetHandler(OpCode::JMPEQ, JumpTableControlJump::JMPEQ);
    SetHandler(OpCode::JMPEQ_L, JumpTableControlJump::JMPEQ_L);
    SetHandler(OpCode::JMPNE, JumpTableControlJump::JMPNE);
    SetHandler(OpCode::JMPNE_L, JumpTableControlJump::JMPNE_L);
    SetHandler(OpCode::JMPGT, JumpTableControlJump::JMPGT);
    SetHandler(OpCode::JMPGT_L, JumpTableControlJump::JMPGT_L);
    SetHandler(OpCode::JMPGE, JumpTableControlJump::JMPGE);
    SetHandler(OpCode::JMPGE_L, JumpTableControlJump::JMPGE_L);
    SetHandler(OpCode::JMPLT, JumpTableControlJump::JMPLT);
    SetHandler(OpCode::JMPLT_L, JumpTableControlJump::JMPLT_L);
    SetHandler(OpCode::JMPLE, JumpTableControlJump::JMPLE);
    SetHandler(OpCode::JMPLE_L, JumpTableControlJump::JMPLE_L);
    SetHandler(OpCode::CALL, JumpTableControlJump::CALL);
    SetHandler(OpCode::CALL_L, JumpTableControlJump::CALL_L);
    SetHandler(OpCode::CALLA, JumpTableControlJump::CALLA);
    SetHandler(OpCode::RET, JumpTableControlJump::RET);
    SetHandler(OpCode::SYSCALL, JumpTableControlJump::SYSCALL);

    // Flow control - Exception handling operations
    // SetHandler(OpCode::LEAVE, JumpTableControlException::LEAVE);
    // SetHandler(OpCode::LEAVE_L, JumpTableControlException::LEAVE_L);
    SetHandler(OpCode::ABORT, JumpTableControlException::ABORT);
    SetHandler(OpCode::ASSERT, JumpTableControlException::ASSERT);
    SetHandler(OpCode::THROW, JumpTableControlException::THROW);
    SetHandler(OpCode::TRY, JumpTableControlException::TRY);
    SetHandler(OpCode::TRY_L, JumpTableControlException::TRY_L);
    SetHandler(OpCode::ENDTRY, JumpTableControlException::ENDTRY);
    SetHandler(OpCode::ENDTRY_L, JumpTableControlException::ENDTRY_L);
    SetHandler(OpCode::ENDFINALLY, JumpTableControlException::ENDFINALLY);

    // Stack operations
    SetHandler(OpCode::DEPTH, JumpTableStack::DEPTH);
    SetHandler(OpCode::DROP, JumpTableStack::DROP);
    SetHandler(OpCode::NIP, JumpTableStack::NIP);
    SetHandler(OpCode::XDROP, JumpTableStack::XDROP);
    SetHandler(OpCode::CLEAR, JumpTableStack::CLEAR);
    SetHandler(OpCode::DUP, JumpTableStack::DUP);
    SetHandler(OpCode::OVER, JumpTableStack::OVER);
    SetHandler(OpCode::PICK, JumpTableStack::PICK);
    SetHandler(OpCode::TUCK, JumpTableStack::TUCK);
    SetHandler(OpCode::SWAP, JumpTableStack::SWAP);
    SetHandler(OpCode::ROT, JumpTableStack::ROT);
    SetHandler(OpCode::ROLL, JumpTableStack::ROLL);
    SetHandler(OpCode::REVERSE3, JumpTableStack::REVERSE3);
    SetHandler(OpCode::REVERSE4, JumpTableStack::REVERSE4);
    SetHandler(OpCode::REVERSEN, JumpTableStack::REVERSEN);

    // Arithmetic operations
    SetHandler(OpCode::ADD,
               [this](ExecutionEngine& engine, const Instruction& instruction) { ADD(engine, instruction); });
    SetHandler(OpCode::SUB,
               [this](ExecutionEngine& engine, const Instruction& instruction) { SUB(engine, instruction); });
    SetHandler(OpCode::MUL,
               [this](ExecutionEngine& engine, const Instruction& instruction) { MUL(engine, instruction); });
    SetHandler(OpCode::DIV,
               [this](ExecutionEngine& engine, const Instruction& instruction) { DIV(engine, instruction); });
    SetHandler(OpCode::MOD,
               [this](ExecutionEngine& engine, const Instruction& instruction) { MOD(engine, instruction); });
    SetHandler(OpCode::POW,
               [this](ExecutionEngine& engine, const Instruction& instruction) { POW(engine, instruction); });
    SetHandler(OpCode::SQRT,
               [this](ExecutionEngine& engine, const Instruction& instruction) { SQRT(engine, instruction); });
    SetHandler(OpCode::SHL,
               [this](ExecutionEngine& engine, const Instruction& instruction) { SHL(engine, instruction); });
    SetHandler(OpCode::SHR,
               [this](ExecutionEngine& engine, const Instruction& instruction) { SHR(engine, instruction); });
    SetHandler(OpCode::NOT,
               [this](ExecutionEngine& engine, const Instruction& instruction) { NOT(engine, instruction); });
    SetHandler(OpCode::BOOLAND,
               [this](ExecutionEngine& engine, const Instruction& instruction) { BOOLAND(engine, instruction); });
    SetHandler(OpCode::BOOLOR,
               [this](ExecutionEngine& engine, const Instruction& instruction) { BOOLOR(engine, instruction); });
    SetHandler(OpCode::NUMEQUAL,
               [this](ExecutionEngine& engine, const Instruction& instruction) { NUMEQUAL(engine, instruction); });
    SetHandler(OpCode::NUMNOTEQUAL,
               [this](ExecutionEngine& engine, const Instruction& instruction) { NUMNOTEQUAL(engine, instruction); });
    SetHandler(OpCode::LT,
               [this](ExecutionEngine& engine, const Instruction& instruction) { LT(engine, instruction); });
    SetHandler(OpCode::GT,
               [this](ExecutionEngine& engine, const Instruction& instruction) { GT(engine, instruction); });
    SetHandler(OpCode::LE,
               [this](ExecutionEngine& engine, const Instruction& instruction) { LE(engine, instruction); });
    SetHandler(OpCode::GE,
               [this](ExecutionEngine& engine, const Instruction& instruction) { GE(engine, instruction); });
    SetHandler(OpCode::MIN,
               [this](ExecutionEngine& engine, const Instruction& instruction) { MIN(engine, instruction); });
    SetHandler(OpCode::MAX,
               [this](ExecutionEngine& engine, const Instruction& instruction) { MAX(engine, instruction); });
    SetHandler(OpCode::WITHIN,
               [this](ExecutionEngine& engine, const Instruction& instruction) { WITHIN(engine, instruction); });
    SetHandler(OpCode::SIGN,
               [this](ExecutionEngine& engine, const Instruction& instruction) { SIGN(engine, instruction); });
    SetHandler(OpCode::ABS,
               [this](ExecutionEngine& engine, const Instruction& instruction) { ABS(engine, instruction); });
    SetHandler(OpCode::NEGATE,
               [this](ExecutionEngine& engine, const Instruction& instruction) { NEGATE(engine, instruction); });
    SetHandler(OpCode::INC,
               [this](ExecutionEngine& engine, const Instruction& instruction) { INC(engine, instruction); });
    SetHandler(OpCode::DEC,
               [this](ExecutionEngine& engine, const Instruction& instruction) { DEC(engine, instruction); });
    SetHandler(OpCode::NZ,
               [this](ExecutionEngine& engine, const Instruction& instruction) { NZ(engine, instruction); });
    SetHandler(OpCode::MODMUL,
               [this](ExecutionEngine& engine, const Instruction& instruction) { MODMUL(engine, instruction); });
    SetHandler(OpCode::MODPOW,
               [this](ExecutionEngine& engine, const Instruction& instruction) { MODPOW(engine, instruction); });

    // Compound type operations
    SetHandler(OpCode::PACK, JumpTableCompound::PACK);
    SetHandler(OpCode::UNPACK, JumpTableCompound::UNPACK);
    SetHandler(OpCode::NEWARRAY0, JumpTableCompound::NEWARRAY0);
    SetHandler(OpCode::NEWARRAY, JumpTableCompound::NEWARRAY);
    SetHandler(OpCode::NEWARRAY_T, JumpTableCompound::NEWARRAY_T);
    SetHandler(OpCode::NEWSTRUCT0, JumpTableCompound::NEWSTRUCT0);
    SetHandler(OpCode::NEWSTRUCT, JumpTableCompound::NEWSTRUCT);
    SetHandler(OpCode::NEWMAP, JumpTableCompound::NEWMAP);
    SetHandler(OpCode::SIZE, JumpTableCompound::SIZE);
    SetHandler(OpCode::HASKEY, JumpTableCompound::HASKEY);
    SetHandler(OpCode::KEYS, JumpTableCompound::KEYS);
    SetHandler(OpCode::VALUES, JumpTableCompound::VALUES);
    SetHandler(OpCode::PICKITEM, JumpTableCompound::PICKITEM);
    SetHandler(OpCode::APPEND, JumpTableCompound::APPEND);
    SetHandler(OpCode::SETITEM, JumpTableCompound::SETITEM);
    SetHandler(OpCode::REMOVE, JumpTableCompound::REMOVE);
    SetHandler(OpCode::CLEARITEMS, JumpTableCompound::CLEARITEMS);
    SetHandler(OpCode::REVERSEITEMS, JumpTableCompound::REVERSEITEMS);
    SetHandler(OpCode::POPITEM, JumpTableCompound::POPITEM);
    SetHandler(OpCode::PACKMAP, JumpTableCompound::PACKMAP);
    SetHandler(OpCode::PACKSTRUCT, JumpTableCompound::PACKSTRUCT);

    // Slot operations
    SetHandler(OpCode::INITSSLOT, JumpTableSlot::INITSSLOT);
    SetHandler(OpCode::INITSLOT, JumpTableSlot::INITSLOT);
    SetHandler(OpCode::LDSFLD0, JumpTableSlot::LDSFLD0);
    SetHandler(OpCode::LDSFLD1, JumpTableSlot::LDSFLD1);
    SetHandler(OpCode::LDSFLD2, JumpTableSlot::LDSFLD2);
    SetHandler(OpCode::LDSFLD3, JumpTableSlot::LDSFLD3);
    SetHandler(OpCode::LDSFLD4, JumpTableSlot::LDSFLD4);
    SetHandler(OpCode::LDSFLD5, JumpTableSlot::LDSFLD5);
    SetHandler(OpCode::LDSFLD6, JumpTableSlot::LDSFLD6);
    SetHandler(OpCode::LDSFLD, JumpTableSlot::LDSFLD);
    SetHandler(OpCode::STSFLD0, JumpTableSlot::STSFLD0);
    SetHandler(OpCode::STSFLD1, JumpTableSlot::STSFLD1);
    SetHandler(OpCode::STSFLD2, JumpTableSlot::STSFLD2);
    SetHandler(OpCode::STSFLD3, JumpTableSlot::STSFLD3);
    SetHandler(OpCode::STSFLD4, JumpTableSlot::STSFLD4);
    SetHandler(OpCode::STSFLD5, JumpTableSlot::STSFLD5);
    SetHandler(OpCode::STSFLD6, JumpTableSlot::STSFLD6);
    SetHandler(OpCode::STSFLD, JumpTableSlot::STSFLD);
    SetHandler(OpCode::LDLOC0, JumpTableSlot::LDLOC0);
    SetHandler(OpCode::LDLOC1, JumpTableSlot::LDLOC1);
    SetHandler(OpCode::LDLOC2, JumpTableSlot::LDLOC2);
    SetHandler(OpCode::LDLOC3, JumpTableSlot::LDLOC3);
    SetHandler(OpCode::LDLOC4, JumpTableSlot::LDLOC4);
    SetHandler(OpCode::LDLOC5, JumpTableSlot::LDLOC5);
    SetHandler(OpCode::LDLOC6, JumpTableSlot::LDLOC6);
    SetHandler(OpCode::LDLOC, JumpTableSlot::LDLOC);
    SetHandler(OpCode::STLOC0, JumpTableSlot::STLOC0);
    SetHandler(OpCode::STLOC1, JumpTableSlot::STLOC1);
    SetHandler(OpCode::STLOC2, JumpTableSlot::STLOC2);
    SetHandler(OpCode::STLOC3, JumpTableSlot::STLOC3);
    SetHandler(OpCode::STLOC4, JumpTableSlot::STLOC4);
    SetHandler(OpCode::STLOC5, JumpTableSlot::STLOC5);
    SetHandler(OpCode::STLOC6, JumpTableSlot::STLOC6);
    SetHandler(OpCode::STLOC, JumpTableSlot::STLOC);
    SetHandler(OpCode::LDARG0, JumpTableSlot::LDARG0);
    SetHandler(OpCode::LDARG1, JumpTableSlot::LDARG1);
    SetHandler(OpCode::LDARG2, JumpTableSlot::LDARG2);
    SetHandler(OpCode::LDARG3, JumpTableSlot::LDARG3);
    SetHandler(OpCode::LDARG4, JumpTableSlot::LDARG4);
    SetHandler(OpCode::LDARG5, JumpTableSlot::LDARG5);
    SetHandler(OpCode::LDARG6, JumpTableSlot::LDARG6);
    SetHandler(OpCode::LDARG, JumpTableSlot::LDARG);
    SetHandler(OpCode::STARG0, JumpTableSlot::STARG0);
    SetHandler(OpCode::STARG1, JumpTableSlot::STARG1);
    SetHandler(OpCode::STARG2, JumpTableSlot::STARG2);
    SetHandler(OpCode::STARG3, JumpTableSlot::STARG3);
    SetHandler(OpCode::STARG4, JumpTableSlot::STARG4);
    SetHandler(OpCode::STARG5, JumpTableSlot::STARG5);
    SetHandler(OpCode::STARG6, JumpTableSlot::STARG6);
    SetHandler(OpCode::STARG, JumpTableSlot::STARG);

    // Splice operations
    SetHandler(OpCode::NEWBUFFER, JumpTableSplice::NEWBUFFER);
    SetHandler(OpCode::MEMCPY, JumpTableSplice::MEMCPY);
    SetHandler(OpCode::CAT, JumpTableSplice::CAT);
    SetHandler(OpCode::SUBSTR, JumpTableSplice::SUBSTR);
    SetHandler(OpCode::LEFT, JumpTableSplice::LEFT);
    SetHandler(OpCode::RIGHT, JumpTableSplice::RIGHT);

    // Bitwise operations
    SetHandler(OpCode::INVERT, JumpTableArithmetic::INVERT);
    SetHandler(OpCode::AND, JumpTableArithmetic::AND);
    SetHandler(OpCode::OR, JumpTableArithmetic::OR);
    SetHandler(OpCode::XOR, JumpTableArithmetic::XOR);
    SetHandler(OpCode::EQUAL, JumpTableArithmetic::EQUAL);
    SetHandler(OpCode::NOTEQUAL, JumpTableArithmetic::NOTEQUAL);

    // Type operations
    SetHandler(OpCode::ISNULL, JumpTableType::ISNULL);
    SetHandler(OpCode::ISTYPE, JumpTableType::ISTYPE);
    SetHandler(OpCode::CONVERT, JumpTableType::CONVERT);

    // Extension operations
    SetHandler(OpCode::ABORTMSG, JumpTableExtension::ABORTMSG);
    SetHandler(OpCode::ASSERTMSG, JumpTableExtension::ASSERTMSG);
    SetHandler(OpCode::PUSHT, JumpTableExtension::PUSHT);
    SetHandler(OpCode::PUSHF, JumpTableExtension::PUSHF);
    SetHandler(OpCode::CALLT, JumpTableExtension::CALLT);
}

const JumpTable::OpcodeHandler& JumpTable::operator[](OpCode opcode) const
{
    return handlers_[static_cast<uint8_t>(opcode)];
}

void JumpTable::SetHandler(OpCode opcode, OpcodeHandler handler)
{
    handlers_[static_cast<uint8_t>(opcode)] = std::move(handler);
}

void JumpTable::InvalidOpcode(ExecutionEngine& engine, const Instruction& instruction)
{
    throw InvalidOperationException("Invalid opcode: " + GetOpCodeName(instruction.opcode));
}

void JumpTable::ExecuteCall(ExecutionEngine& engine, int32_t position)
{
    auto& context = engine.GetCurrentContext();
    auto clonedContext = context.Clone(position);
    engine.LoadContext(clonedContext);
}

void JumpTable::ExecuteJump(ExecutionEngine& engine, int32_t position)
{
    auto& context = engine.GetCurrentContext();
    if (position < 0 || position >= static_cast<int32_t>(context.GetScript().GetLength()))
        throw std::out_of_range("Jump out of range");

    context.SetInstructionPointer(position);
    engine.SetJumping(true);
}

void JumpTable::ExecuteJumpOffset(ExecutionEngine& engine, int32_t offset)
{
    auto& context = engine.GetCurrentContext();
    int32_t position = context.GetInstructionPointer() + offset;
    ExecuteJump(engine, position);
}

void JumpTable::ExecuteTry(ExecutionEngine& engine, int32_t catchOffset, int32_t finallyOffset)
{
    if (catchOffset == 0 && finallyOffset == 0)
        throw InvalidOperationException("catchOffset and finallyOffset can't be 0 in a TRY block");

    auto& context = engine.GetCurrentContext();
    auto instructionPointer = context.GetInstructionPointer();

    int32_t catchPointer = catchOffset == 0 ? -1 : instructionPointer + catchOffset;
    int32_t finallyPointer = finallyOffset == 0 ? -1 : instructionPointer + finallyOffset;

    // The end pointer will be set by the ENDTRY instruction
    context.EnterTry(catchPointer, finallyPointer, 0);
}

void JumpTable::ExecuteEndTry(ExecutionEngine& engine, int32_t endOffset)
{
    auto& context = engine.GetCurrentContext();
    if (context.GetTryCount() == 0) throw InvalidOperationException("The corresponding TRY block cannot be found");

    auto catchOffset = context.GetCatchOffset();
    auto finallyOffset = context.GetFinallyOffset();
    auto endPointer = context.GetInstructionPointer() + endOffset;

    // Set the end pointer in the current try context
    auto& tryContext = context.GetCurrentTry();
    tryContext.SetEndPointer(endPointer);

    if (finallyOffset.has_value())
    {
        // Set the state to Finally
        tryContext.SetState(ExceptionHandlingState::Finally);
        context.SetInstructionPointer(finallyOffset.value());
    }
    else
    {
        context.ExitTry();
        context.SetInstructionPointer(endPointer);
    }

    engine.SetJumping(true);
}

}  // namespace neo::vm
