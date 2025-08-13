/**
 * @file jump_table_declarations.h
 * @brief Jump Table Declarations
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

namespace neo::vm
{
class JumpTable;

// Flow control operations
void JumpTable::JMP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMP_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPIF(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPIF_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPIFNOT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPIFNOT_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPEQ(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPEQ_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPNE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPNE_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPGT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPGT_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPGE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPGE_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPLT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPLT_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPLE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::JMPLE_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::CALL(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::CALL_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::CALLA(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ABORT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ASSERT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::THROW(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::TRY(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::TRY_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ENDTRY(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ENDTRY_L(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ENDFINALLY(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::RET(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SYSCALL(ExecutionEngine& engine, const Instruction& instruction);

// Stack operations
void JumpTable::DEPTH(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::DROP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NIP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::XDROP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::CLEAR(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::DUP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::OVER(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::PICK(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::TUCK(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SWAP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ROT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::ROLL(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::REVERSE3(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::REVERSE4(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::REVERSEN(ExecutionEngine& engine, const Instruction& instruction);

// Arithmetic operations
void JumpTable::ADD(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SUB(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::MUL(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::DIV(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::MOD(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::POW(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SQRT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SHL(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SHR(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NOT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::BOOLAND(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::BOOLOR(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NUMEQUAL(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NUMNOTEQUAL(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::LT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::GT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::LTE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::GTE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::MIN(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::MAX(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::WITHIN(ExecutionEngine& engine, const Instruction& instruction);

// Compound type operations
void JumpTable::PACK(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::UNPACK(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NEWARRAY0(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NEWARRAY(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NEWARRAY_T(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NEWSTRUCT0(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NEWSTRUCT(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::NEWMAP(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SIZE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::HASKEY(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::KEYS(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::VALUES(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::PICKITEM(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::APPEND(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::SETITEM(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::REMOVE(ExecutionEngine& engine, const Instruction& instruction);
void JumpTable::CLEARITEMS(ExecutionEngine& engine, const Instruction& instruction);
}  // namespace neo::vm
