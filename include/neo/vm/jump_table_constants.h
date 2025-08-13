/**
 * @file jump_table_constants.h
 * @brief Constant definitions
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/vm/jump_table.h>

namespace neo::vm
{
/**
 * @brief Constants-related opcode handlers for the JumpTable.
 */
class JumpTableConstants
{
   public:
    /**
     * @brief Pushes a 1-byte signed integer onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHINT8(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes a 2-byte signed integer onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHINT16(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes a 4-byte signed integer onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHINT32(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes a 8-byte signed integer onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHINT64(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes a 16-byte signed integer onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHINT128(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes a 32-byte signed integer onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHINT256(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes the address of the specified instruction onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHA(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes null onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHNULL(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes data with a 1-byte length prefix onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHDATA1(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes data with a 2-byte length prefix onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHDATA2(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes data with a 4-byte length prefix onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHDATA4(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes -1 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSHM1(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 0 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH0(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 1 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH1(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 2 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH2(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 3 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH3(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 4 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH4(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 5 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH5(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 6 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH6(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 7 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH7(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 8 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH8(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 9 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH9(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 10 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH10(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 11 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH11(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 12 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH12(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 13 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH13(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 14 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH14(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 15 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH15(ExecutionEngine& engine, const Instruction& instruction);

    /**
     * @brief Pushes 16 onto the stack.
     * @param engine The execution engine.
     * @param instruction The instruction.
     */
    static void PUSH16(ExecutionEngine& engine, const Instruction& instruction);
};
}  // namespace neo::vm
