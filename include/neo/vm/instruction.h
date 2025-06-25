#pragma once

#include <neo/vm/opcode.h>
#include <neo/vm/internal/byte_vector.h>
#include <neo/vm/internal/byte_span.h>
#include <cstdint>
#include <memory>
#include <array>
#include <stdexcept>

namespace neo::vm
{
    /**
     * @brief Represents instructions in the VM script.
     */
    class Instruction
    {
    public:
        /**
         * @brief Static instance of the RET instruction.
         */
        static const Instruction RET;

        /**
         * @brief The OpCode of the instruction.
         */
        const neo::vm::OpCode opcode;

        /**
         * @brief The operand of the instruction.
         */
        const internal::ByteVector Operand;

        /**
         * @brief Constructs a new instruction with the specified opcode.
         * @param opcode The opcode.
         */
        explicit Instruction(vm::OpCode opcode);

        /**
         * @brief Constructs a new instruction with the specified opcode and operand.
         * @param opcode The opcode.
         * @param operand The operand.
         */
        Instruction(vm::OpCode opcode, const internal::ByteVector& operand);

        /**
         * @brief Constructs a new instruction from a script at the specified position.
         * @param script The script.
         * @param ip The instruction pointer.
         * @throws std::runtime_error If the instruction is invalid or out of bounds.
         */
        Instruction(const internal::ByteSpan& script, int ip);

        /**
         * @brief Gets the size of the instruction.
         * @return The size of the instruction.
         */
        int Size() const;

        /**
         * @brief Gets the first operand as int16_t.
         * @return The first operand as int16_t.
         */
        int16_t TokenI16() const;

        /**
         * @brief Gets the first operand as int32_t.
         * @return The first operand as int32_t.
         */
        int32_t TokenI32() const;

        /**
         * @brief Gets the first operand as int64_t.
         * @return The first operand as int64_t.
         */
        int64_t TokenI64() const;

        /**
         * @brief Gets the first operand as uint8_t.
         * @return The first operand as uint8_t.
         */
        uint8_t TokenU8() const;

        /**
         * @brief Gets the second operand as uint8_t.
         * @return The second operand as uint8_t.
         */
        uint8_t TokenU8_1() const;

        /**
         * @brief Gets the first operand as int8_t.
         * @return The first operand as int8_t.
         */
        int8_t TokenI8() const;

        /**
         * @brief Gets the second operand as int8_t.
         * @return The second operand as int8_t.
         */
        int8_t TokenI8_1() const;

        /**
         * @brief Gets the first operand as uint16_t.
         * @return The first operand as uint16_t.
         */
        uint16_t TokenU16() const;

        /**
         * @brief Gets the first operand as uint32_t.
         * @return The first operand as uint32_t.
         */
        uint32_t TokenU32() const;

        /**
         * @brief Gets the first operand as int32_t.
         * @return The first operand as int32_t.
         */
        int32_t TokenI32_1() const;

        /**
         * @brief Gets the operand as an integer value.
         * @return The operand as an integer value.
         */
        int64_t GetOperand() const;

        /**
         * @brief Gets the operand data.
         * @return The operand data.
         */
        internal::ByteVector GetData() const;

    private:
        static std::array<int, 256> OperandSizePrefixTable;
        static std::array<int, 256> OperandSizeTable;

        static void InitializeOperandSizeTables();
    };
}
