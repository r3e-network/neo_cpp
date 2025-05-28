#include <neo/vm/script.h>
#include <neo/vm/exceptions.h>
#include <unordered_map>
#include <stdexcept>

namespace neo::vm
{
    // Script implementation
    Script::Script() = default;

    Script::Script(const internal::ByteVector& script)
        : script_(script)
    {
    }

    Script::Script(const internal::ByteSpan& script)
        : script_(script)
    {
    }

    const internal::ByteVector& Script::GetScript() const
    {
        return script_;
    }

    size_t Script::GetLength() const
    {
        return script_.Size();
    }

    std::shared_ptr<Instruction> Script::GetInstruction(int32_t position) const
    {
        if (position < 0 || position >= static_cast<int32_t>(script_.Size()))
            return nullptr;

        // Check if the instruction is already cached
        auto it = instructions_.find(position);
        if (it != instructions_.end())
            return it->second;

        try
        {
            // Create a new instruction
            auto instruction = std::make_shared<Instruction>(script_.AsSpan(), position);
            instructions_[position] = instruction;
            return instruction;
        }
        catch (const BadScriptException&)
        {
            return nullptr;
        }
    }

    std::shared_ptr<Instruction> Script::GetNextInstruction(int32_t& position) const
    {
        auto instruction = GetInstruction(position);
        if (!instruction)
            return nullptr;

        position += instruction->Size();
        return instruction;
    }

    int32_t Script::GetJumpDestination(int32_t position, int32_t offset) const
    {
        return position + offset;
    }

    // Serialization methods removed to eliminate dependencies on the IO module

    bool Script::operator==(const Script& other) const
    {
        return script_ == other.script_;
    }

    bool Script::operator!=(const Script& other) const
    {
        return !(*this == other);
    }

    int32_t Script::GetOperandSize(OpCode opcode)
    {
        switch (opcode)
        {
            case OpCode::PUSHINT8:
                return 1;
            case OpCode::PUSHINT16:
                return 2;
            case OpCode::PUSHINT32:
                return 4;
            case OpCode::PUSHINT64:
                return 8;
            case OpCode::PUSHINT128:
                return 16;
            case OpCode::PUSHINT256:
                return 32;
            case OpCode::PUSHA:
                return 4;
            case OpCode::PUSHDATA1:
            case OpCode::PUSHDATA2:
            case OpCode::PUSHDATA4:
                return 0; // Variable length
            case OpCode::JMP:
            case OpCode::JMPIF:
            case OpCode::JMPIFNOT:
            case OpCode::JMPEQ:
            case OpCode::JMPNE:
            case OpCode::JMPGT:
            case OpCode::JMPGE:
            case OpCode::JMPLT:
            case OpCode::JMPLE:
            case OpCode::CALL:
                return 1;
            case OpCode::JMP_L:
            case OpCode::JMPIF_L:
            case OpCode::JMPIFNOT_L:
            case OpCode::JMPEQ_L:
            case OpCode::JMPNE_L:
            case OpCode::JMPGT_L:
            case OpCode::JMPGE_L:
            case OpCode::JMPLT_L:
            case OpCode::JMPLE_L:
            case OpCode::CALL_L:
            case OpCode::LDSFLD:
            case OpCode::STSFLD:
            case OpCode::LDLOC:
            case OpCode::STLOC:
            case OpCode::LDARG:
            case OpCode::STARG:
            case OpCode::NEWARRAY_T:
            case OpCode::ISTYPE:
            case OpCode::CONVERT:
                return 1;
            case OpCode::INITSLOT:
                return 2;
            case OpCode::TRY:
                return 2;
            case OpCode::TRY_L:
                return 8;
            case OpCode::ENDTRY:
                return 1;
            case OpCode::ENDTRY_L:
                return 4;
            case OpCode::SYSCALL:
                return 4;
            default:
                return 0;
        }
    }

    int64_t Script::GetPrice(OpCode opcode)
    {
        // Gas pricing based on C# Neo implementation
        // In the unit of datoshi, 1 datoshi = 1e-8 GAS
        switch (opcode)
        {
            // Push operations - very low cost
            case OpCode::PUSHINT8:
            case OpCode::PUSHINT16:
            case OpCode::PUSHINT32:
            case OpCode::PUSHINT64:
            case OpCode::PUSHT:
            case OpCode::PUSHF:
            case OpCode::PUSHNULL:
            case OpCode::PUSHM1:
            case OpCode::PUSH0:
            case OpCode::PUSH1:
            case OpCode::PUSH2:
            case OpCode::PUSH3:
            case OpCode::PUSH4:
            case OpCode::PUSH5:
            case OpCode::PUSH6:
            case OpCode::PUSH7:
            case OpCode::PUSH8:
            case OpCode::PUSH9:
            case OpCode::PUSH10:
            case OpCode::PUSH11:
            case OpCode::PUSH12:
            case OpCode::PUSH13:
            case OpCode::PUSH14:
            case OpCode::PUSH15:
            case OpCode::PUSH16:
                return 1 << 0; // 1 gas

            // Larger push operations
            case OpCode::PUSHINT128:
            case OpCode::PUSHINT256:
            case OpCode::PUSHA:
                return 1 << 2; // 4 gas

            // Push data operations
            case OpCode::PUSHDATA1:
                return 1 << 3; // 8 gas
            case OpCode::PUSHDATA2:
                return 1 << 9; // 512 gas
            case OpCode::PUSHDATA4:
                return 1 << 12; // 4096 gas

            // Arithmetic operations
            case OpCode::SIGN:
            case OpCode::ABS:
            case OpCode::NEGATE:
            case OpCode::INC:
            case OpCode::DEC:
            case OpCode::NOT:
            case OpCode::NZ:
                return 1 << 2; // 4 gas

            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
            case OpCode::MOD:
            case OpCode::SHL:
            case OpCode::SHR:
            case OpCode::BOOLAND:
            case OpCode::BOOLOR:
            case OpCode::NUMEQUAL:
            case OpCode::NUMNOTEQUAL:
            case OpCode::LT:
            case OpCode::LE:
            case OpCode::GT:
            case OpCode::GE:
            case OpCode::AND:
            case OpCode::OR:
            case OpCode::XOR:
                return 1 << 3; // 8 gas

            // More expensive operations
            case OpCode::EQUAL:
            case OpCode::NOTEQUAL:
            case OpCode::MODMUL:
                return 1 << 5; // 32 gas

            case OpCode::POW:
            case OpCode::SQRT:
                return 1 << 6; // 64 gas

            case OpCode::MODPOW:
                return 1 << 11; // 2048 gas

            // Type conversion
            case OpCode::CONVERT:
                return 1 << 13; // 8192 gas

            // Default for other operations
            default:
                return 1 << 0; // 1 gas
        }
    }

    std::string Script::GetOpCodeName(OpCode opcode)
    {
        return vm::GetOpCodeName(opcode);
    }
}
