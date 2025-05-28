#pragma once

#include <neo/vm/opcode.h>
#include <neo/vm/instruction.h>
#include <functional>
#include <array>
#include <memory>

// Forward declarations
namespace neo::vm
{
    class ExecutionEngine;
    class StackItem;
}

namespace neo::vm
{
    /**
     * @brief Represents a jump table for the VM.
     *
     * The JumpTable maps opcodes to their implementation functions.
     */
    class JumpTable
    {
    public:
        /**
         * @brief Delegate type for opcode handlers.
         */
        using OpcodeHandler = std::function<void(ExecutionEngine&, const Instruction&)>;

        /**
         * @brief Constructs a new JumpTable.
         */
        JumpTable();

        /**
         * @brief Gets the handler for the specified opcode.
         * @param opcode The opcode.
         * @return The handler.
         */
        const OpcodeHandler& operator[](OpCode opcode) const;

        /**
         * @brief Sets the handler for the specified opcode.
         * @param opcode The opcode.
         * @param handler The handler.
         */
        void SetHandler(OpCode opcode, OpcodeHandler handler);

        /**
         * @brief Default JumpTable instance.
         */
        static const JumpTable Default;

        // Control flow operations

        /**
         * @brief Executes a call operation.
         * @param engine The execution engine.
         * @param position The position to call.
         */
        static void ExecuteCall(ExecutionEngine& engine, int32_t position);

        /**
         * @brief Executes a jump operation.
         * @param engine The execution engine.
         * @param position The position to jump to.
         */
        static void ExecuteJump(ExecutionEngine& engine, int32_t position);

        /**
         * @brief Executes a jump offset operation.
         * @param engine The execution engine.
         * @param offset The offset to jump.
         */
        static void ExecuteJumpOffset(ExecutionEngine& engine, int32_t offset);

        /**
         * @brief Executes a try operation.
         * @param engine The execution engine.
         * @param catchOffset The catch offset.
         * @param finallyOffset The finally offset.
         */
        static void ExecuteTry(ExecutionEngine& engine, int32_t catchOffset, int32_t finallyOffset);

        /**
         * @brief Executes an end try operation.
         * @param engine The execution engine.
         * @param endOffset The end offset.
         */
        static void ExecuteEndTry(ExecutionEngine& engine, int32_t endOffset);

        /**
         * @brief Executes a throw operation.
         * @param engine The execution engine.
         * @param exception The exception to throw.
         */
        static void ExecuteThrow(ExecutionEngine& engine, const std::string& message);

        /**
         * @brief Executes a throw operation.
         * @param engine The execution engine.
         * @param exception The exception to throw.
         */
        static void ExecuteThrow(ExecutionEngine& engine, std::shared_ptr<StackItem> exception);

        // Opcode handlers

        /**
         * @brief Handler for invalid opcodes.
         * @param engine The execution engine.
         * @param instruction The instruction.
         */
        static void InvalidOpcode(ExecutionEngine& engine, const Instruction& instruction);

        // Flow control operations
        static void JMP(ExecutionEngine& engine, const Instruction& instruction);
        static void JMP_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIF(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIF_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIFNOT(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPIFNOT_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPEQ(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPEQ_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPNE(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPNE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGT(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGT_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGE(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPGE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLT(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLT_L(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLE(ExecutionEngine& engine, const Instruction& instruction);
        static void JMPLE_L(ExecutionEngine& engine, const Instruction& instruction);
        static void CALL(ExecutionEngine& engine, const Instruction& instruction);
        static void CALL_L(ExecutionEngine& engine, const Instruction& instruction);
        static void CALLA(ExecutionEngine& engine, const Instruction& instruction);
        static void ABORT(ExecutionEngine& engine, const Instruction& instruction);
        static void ASSERT(ExecutionEngine& engine, const Instruction& instruction);
        static void THROW(ExecutionEngine& engine, const Instruction& instruction);
        static void TRY(ExecutionEngine& engine, const Instruction& instruction);
        static void TRY_L(ExecutionEngine& engine, const Instruction& instruction);
        static void ENDTRY(ExecutionEngine& engine, const Instruction& instruction);
        static void ENDTRY_L(ExecutionEngine& engine, const Instruction& instruction);
        static void ENDFINALLY(ExecutionEngine& engine, const Instruction& instruction);
        static void RET(ExecutionEngine& engine, const Instruction& instruction);
        static void SYSCALL(ExecutionEngine& engine, const Instruction& instruction);
        static void LEAVE(ExecutionEngine& engine, const Instruction& instruction);
        static void LEAVE_L(ExecutionEngine& engine, const Instruction& instruction);

        // Stack operations
        static void DEPTH(ExecutionEngine& engine, const Instruction& instruction);
        static void DROP(ExecutionEngine& engine, const Instruction& instruction);
        static void NIP(ExecutionEngine& engine, const Instruction& instruction);
        static void XDROP(ExecutionEngine& engine, const Instruction& instruction);
        static void CLEAR(ExecutionEngine& engine, const Instruction& instruction);
        static void DUP(ExecutionEngine& engine, const Instruction& instruction);
        static void OVER(ExecutionEngine& engine, const Instruction& instruction);
        static void PICK(ExecutionEngine& engine, const Instruction& instruction);
        static void TUCK(ExecutionEngine& engine, const Instruction& instruction);
        static void SWAP(ExecutionEngine& engine, const Instruction& instruction);
        static void ROT(ExecutionEngine& engine, const Instruction& instruction);
        static void ROLL(ExecutionEngine& engine, const Instruction& instruction);
        static void REVERSE3(ExecutionEngine& engine, const Instruction& instruction);
        static void REVERSE4(ExecutionEngine& engine, const Instruction& instruction);
        static void REVERSEN(ExecutionEngine& engine, const Instruction& instruction);

        // Arithmetic operations
        static void ADD(ExecutionEngine& engine, const Instruction& instruction);
        static void SUB(ExecutionEngine& engine, const Instruction& instruction);
        static void MUL(ExecutionEngine& engine, const Instruction& instruction);
        static void DIV(ExecutionEngine& engine, const Instruction& instruction);
        static void MOD(ExecutionEngine& engine, const Instruction& instruction);
        static void POW(ExecutionEngine& engine, const Instruction& instruction);
        static void SQRT(ExecutionEngine& engine, const Instruction& instruction);
        static void SHL(ExecutionEngine& engine, const Instruction& instruction);
        static void SHR(ExecutionEngine& engine, const Instruction& instruction);
        static void NOT(ExecutionEngine& engine, const Instruction& instruction);
        static void BOOLAND(ExecutionEngine& engine, const Instruction& instruction);
        static void BOOLOR(ExecutionEngine& engine, const Instruction& instruction);
        static void NUMEQUAL(ExecutionEngine& engine, const Instruction& instruction);
        static void NUMNOTEQUAL(ExecutionEngine& engine, const Instruction& instruction);
        static void LT(ExecutionEngine& engine, const Instruction& instruction);
        static void GT(ExecutionEngine& engine, const Instruction& instruction);
        static void LE(ExecutionEngine& engine, const Instruction& instruction);
        static void GE(ExecutionEngine& engine, const Instruction& instruction);
        static void MIN(ExecutionEngine& engine, const Instruction& instruction);
        static void MAX(ExecutionEngine& engine, const Instruction& instruction);
        static void WITHIN(ExecutionEngine& engine, const Instruction& instruction);
        static void SIGN(ExecutionEngine& engine, const Instruction& instruction);
        static void ABS(ExecutionEngine& engine, const Instruction& instruction);
        static void NEGATE(ExecutionEngine& engine, const Instruction& instruction);
        static void INC(ExecutionEngine& engine, const Instruction& instruction);
        static void DEC(ExecutionEngine& engine, const Instruction& instruction);
        static void INVERT(ExecutionEngine& engine, const Instruction& instruction);
        static void AND(ExecutionEngine& engine, const Instruction& instruction);
        static void OR(ExecutionEngine& engine, const Instruction& instruction);
        static void XOR(ExecutionEngine& engine, const Instruction& instruction);
        static void EQUAL(ExecutionEngine& engine, const Instruction& instruction);
        static void NOTEQUAL(ExecutionEngine& engine, const Instruction& instruction);
        static void MODMUL(ExecutionEngine& engine, const Instruction& instruction);
        static void MODPOW(ExecutionEngine& engine, const Instruction& instruction);
        static void NZ(ExecutionEngine& engine, const Instruction& instruction);
        static void ISNULL(ExecutionEngine& engine, const Instruction& instruction);
        static void ISTYPE(ExecutionEngine& engine, const Instruction& instruction);
        static void CONVERT(ExecutionEngine& engine, const Instruction& instruction);

        // Compound type operations
        static void PACK(ExecutionEngine& engine, const Instruction& instruction);
        static void UNPACK(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWARRAY0(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWARRAY(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWARRAY_T(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWSTRUCT0(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWSTRUCT(ExecutionEngine& engine, const Instruction& instruction);
        static void NEWMAP(ExecutionEngine& engine, const Instruction& instruction);
        static void SIZE(ExecutionEngine& engine, const Instruction& instruction);
        static void HASKEY(ExecutionEngine& engine, const Instruction& instruction);
        static void KEYS(ExecutionEngine& engine, const Instruction& instruction);
        static void VALUES(ExecutionEngine& engine, const Instruction& instruction);
        static void PICKITEM(ExecutionEngine& engine, const Instruction& instruction);
        static void APPEND(ExecutionEngine& engine, const Instruction& instruction);
        static void SETITEM(ExecutionEngine& engine, const Instruction& instruction);
        static void REMOVE(ExecutionEngine& engine, const Instruction& instruction);
        static void CLEARITEMS(ExecutionEngine& engine, const Instruction& instruction);
        static void REVERSEITEMS(ExecutionEngine& engine, const Instruction& instruction);
        static void POPITEM(ExecutionEngine& engine, const Instruction& instruction);
        static void PACKMAP(ExecutionEngine& engine, const Instruction& instruction);
        static void PACKSTRUCT(ExecutionEngine& engine, const Instruction& instruction);

        // Splice operations
        static void NEWBUFFER(ExecutionEngine& engine, const Instruction& instruction);
        static void MEMCPY(ExecutionEngine& engine, const Instruction& instruction);
        static void CAT(ExecutionEngine& engine, const Instruction& instruction);
        static void SUBSTR(ExecutionEngine& engine, const Instruction& instruction);
        static void LEFT(ExecutionEngine& engine, const Instruction& instruction);
        static void RIGHT(ExecutionEngine& engine, const Instruction& instruction);

        // Slot operations
        static void INITSSLOT(ExecutionEngine& engine, const Instruction& instruction);
        static void INITSLOT(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD0(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD1(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD2(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD3(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD4(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD5(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD6(ExecutionEngine& engine, const Instruction& instruction);
        static void LDSFLD(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD0(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD1(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD2(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD3(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD4(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD5(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD6(ExecutionEngine& engine, const Instruction& instruction);
        static void STSFLD(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC0(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC1(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC2(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC3(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC4(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC5(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC6(ExecutionEngine& engine, const Instruction& instruction);
        static void LDLOC(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC0(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC1(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC2(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC3(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC4(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC5(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC6(ExecutionEngine& engine, const Instruction& instruction);
        static void STLOC(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG0(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG1(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG2(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG3(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG4(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG5(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG6(ExecutionEngine& engine, const Instruction& instruction);
        static void LDARG(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG0(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG1(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG2(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG3(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG4(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG5(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG6(ExecutionEngine& engine, const Instruction& instruction);
        static void STARG(ExecutionEngine& engine, const Instruction& instruction);

    private:
        std::array<OpcodeHandler, 256> handlers_;
    };
}
