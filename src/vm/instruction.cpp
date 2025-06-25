#include <neo/vm/instruction.h>
#include <neo/vm/exceptions.h>
#include <cstring>

namespace neo::vm
{
    // Initialize static members
    std::array<int, 256> Instruction::OperandSizePrefixTable = {};
    std::array<int, 256> Instruction::OperandSizeTable = {};

    // Static RET instruction
    const Instruction Instruction::RET(OpCode::RET);

    // Initialize operand size tables
    void Instruction::InitializeOperandSizeTables()
    {
        static bool initialized = false;
        if (initialized) return;

        // Initialize with default values
        OperandSizePrefixTable.fill(0);
        OperandSizeTable.fill(0);

        // Set operand sizes based on OpCode
        // Constants
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHINT8)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHINT16)] = 2;
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHINT32)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHINT64)] = 8;
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHINT128)] = 16;
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHINT256)] = 32;
        OperandSizeTable[static_cast<uint8_t>(OpCode::PUSHA)] = 4;

        // Variable length operands
        OperandSizePrefixTable[static_cast<uint8_t>(OpCode::PUSHDATA1)] = 1;
        OperandSizePrefixTable[static_cast<uint8_t>(OpCode::PUSHDATA2)] = 2;
        OperandSizePrefixTable[static_cast<uint8_t>(OpCode::PUSHDATA4)] = 4;

        // Flow control
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMP)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMP_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPIF)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPIF_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPIFNOT)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPIFNOT_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPEQ)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPEQ_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPNE)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPNE_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPGT)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPGT_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPGE)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPGE_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPLT)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPLT_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPLE)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::JMPLE_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::CALL)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::CALL_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::CALLT)] = 2;
        OperandSizeTable[static_cast<uint8_t>(OpCode::TRY)] = 2;
        OperandSizeTable[static_cast<uint8_t>(OpCode::TRY_L)] = 8;
        OperandSizeTable[static_cast<uint8_t>(OpCode::ENDTRY)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::ENDTRY_L)] = 4;
        OperandSizeTable[static_cast<uint8_t>(OpCode::SYSCALL)] = 4;

        // Stack
        OperandSizeTable[static_cast<uint8_t>(OpCode::INITSLOT)] = 2;

        // Slot
        OperandSizeTable[static_cast<uint8_t>(OpCode::LDSFLD)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::STSFLD)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::LDLOC)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::STLOC)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::LDARG)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::STARG)] = 1;

        // Array/Struct
        OperandSizeTable[static_cast<uint8_t>(OpCode::NEWARRAY_T)] = 1;

        // Type conversion
        OperandSizeTable[static_cast<uint8_t>(OpCode::ISTYPE)] = 1;
        OperandSizeTable[static_cast<uint8_t>(OpCode::CONVERT)] = 1;

        initialized = true;
    }

    Instruction::Instruction(vm::OpCode opcode)
        : opcode(opcode), Operand()
    {
        InitializeOperandSizeTables();
    }

    Instruction::Instruction(vm::OpCode opcode, const internal::ByteVector& operand)
        : opcode(opcode), Operand(operand)
    {
        InitializeOperandSizeTables();
    }

    Instruction::Instruction(const internal::ByteSpan& script, int ip)
        : opcode(static_cast<vm::OpCode>(script[ip++])), Operand()
    {
        InitializeOperandSizeTables();

        int operandSizePrefix = OperandSizePrefixTable[static_cast<uint8_t>(opcode)];
        int operandSize = 0;

        switch (operandSizePrefix)
        {
            case 0:
                operandSize = OperandSizeTable[static_cast<uint8_t>(opcode)];
                break;
            case 1:
                if (ip >= script.Size())
                    throw BadScriptException("Instruction out of bounds");
                operandSize = script[ip];
                break;
            case 2:
                if (ip + 1 >= script.Size())
                    throw BadScriptException("Instruction out of bounds");
                operandSize = script[ip] | (script[ip + 1] << 8);
                break;
            case 4:
                if (ip + 3 >= script.Size())
                    throw BadScriptException("Instruction out of bounds");
                operandSize = script[ip] | (script[ip + 1] << 8) | (script[ip + 2] << 16) | (script[ip + 3] << 24);
                if (operandSize < 0)
                    throw BadScriptException("Invalid operand size");
                break;
        }

        ip += operandSizePrefix;

        if (operandSize > 0)
        {
            if (ip + operandSize > script.Size())
                throw BadScriptException("Instruction out of bounds");

            // Create a new ByteVector with the operand data
            internal::ByteVector operand(script.Slice(ip, operandSize));
            const_cast<internal::ByteVector&>(Operand) = std::move(operand);
        }
    }

    int Instruction::Size() const
    {
        int prefixSize = OperandSizePrefixTable[static_cast<uint8_t>(opcode)];
        return prefixSize > 0
            ? 1 + prefixSize + Operand.Size()
            : 1 + OperandSizeTable[static_cast<uint8_t>(opcode)];
    }

    int16_t Instruction::TokenI16() const
    {
        if (Operand.Size() < 2)
            throw std::runtime_error("Operand too small for TokenI16");

        return static_cast<int16_t>(Operand[0] | (Operand[1] << 8));
    }

    int32_t Instruction::TokenI32() const
    {
        if (Operand.Size() < 4)
            throw std::runtime_error("Operand too small for TokenI32");

        return static_cast<int32_t>(Operand[0] | (Operand[1] << 8) | (Operand[2] << 16) | (Operand[3] << 24));
    }

    int64_t Instruction::TokenI64() const
    {
        if (Operand.Size() < 8)
            throw std::runtime_error("Operand too small for TokenI64");

        return static_cast<int64_t>(
            static_cast<uint64_t>(Operand[0]) |
            (static_cast<uint64_t>(Operand[1]) << 8) |
            (static_cast<uint64_t>(Operand[2]) << 16) |
            (static_cast<uint64_t>(Operand[3]) << 24) |
            (static_cast<uint64_t>(Operand[4]) << 32) |
            (static_cast<uint64_t>(Operand[5]) << 40) |
            (static_cast<uint64_t>(Operand[6]) << 48) |
            (static_cast<uint64_t>(Operand[7]) << 56)
        );
    }

    uint8_t Instruction::TokenU8() const
    {
        if (Operand.IsEmpty())
            throw std::runtime_error("Operand too small for TokenU8");

        return Operand[0];
    }

    uint8_t Instruction::TokenU8_1() const
    {
        if (Operand.Size() < 2)
            throw std::runtime_error("Operand too small for TokenU8_1");

        return Operand[1];
    }

    int8_t Instruction::TokenI8() const
    {
        if (Operand.IsEmpty())
            throw std::runtime_error("Operand too small for TokenI8");

        return static_cast<int8_t>(Operand[0]);
    }

    int8_t Instruction::TokenI8_1() const
    {
        if (Operand.Size() < 2)
            throw std::runtime_error("Operand too small for TokenI8_1");

        return static_cast<int8_t>(Operand[1]);
    }

    uint16_t Instruction::TokenU16() const
    {
        if (Operand.Size() < 2)
            throw std::runtime_error("Operand too small for TokenU16");

        return static_cast<uint16_t>(Operand[0] | (Operand[1] << 8));
    }

    uint32_t Instruction::TokenU32() const
    {
        if (Operand.Size() < 4)
            throw std::runtime_error("Operand too small for TokenU32");

        return static_cast<uint32_t>(Operand[0] | (Operand[1] << 8) | (Operand[2] << 16) | (Operand[3] << 24));
    }

    int32_t Instruction::TokenI32_1() const
    {
        if (Operand.Size() < 8)
            throw std::runtime_error("Operand too small for TokenI32_1");

        return static_cast<int32_t>(Operand[4] | (Operand[5] << 8) | (Operand[6] << 16) | (Operand[7] << 24));
    }

    int64_t Instruction::GetOperand() const
    {
        switch (OperandSizeTable[static_cast<uint8_t>(opcode)])
        {
            case 1:
                return TokenI8();
            case 2:
                return TokenI16();
            case 4:
                return TokenI32();
            case 8:
                return TokenI64();
            default:
                throw std::runtime_error("Invalid operand size for GetOperand");
        }
    }

    internal::ByteVector Instruction::GetData() const
    {
        return Operand;
    }
}
