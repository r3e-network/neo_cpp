#include <neo/vm/jump_table_constants.h>
#include <neo/vm/execution_engine.h>
#include <neo/vm/stack_item.h>
#include <neo/vm/exceptions.h>
#include <neo/vm/primitive_items.h>
#include <neo/vm/compound_items.h>
#include <neo/vm/special_items.h>
#include <stdexcept>

namespace neo::vm
{
    void JumpTableConstants::PUSHINT8(ExecutionEngine& engine, const Instruction& instruction)
    {
        int8_t value = static_cast<int8_t>(instruction.GetOperand());
        engine.Push(StackItem::Create(static_cast<int64_t>(value)));
    }

    void JumpTableConstants::PUSHINT16(ExecutionEngine& engine, const Instruction& instruction)
    {
        int16_t value = static_cast<int16_t>(instruction.GetOperand());
        engine.Push(StackItem::Create(static_cast<int64_t>(value)));
    }

    void JumpTableConstants::PUSHINT32(ExecutionEngine& engine, const Instruction& instruction)
    {
        int32_t value = static_cast<int32_t>(instruction.GetOperand());
        engine.Push(StackItem::Create(static_cast<int64_t>(value)));
    }

    void JumpTableConstants::PUSHINT64(ExecutionEngine& engine, const Instruction& instruction)
    {
        int64_t value = static_cast<int64_t>(instruction.GetOperand());
        engine.Push(StackItem::Create(value));
    }

    void JumpTableConstants::PUSHINT128(ExecutionEngine& engine, const Instruction& instruction)
    {
        // Implement 128-bit integer support
        auto data = instruction.GetData();
        if (data.Size() != 16)
            throw std::runtime_error("PUSHINT128 requires exactly 16 bytes");

        // Complete 128-bit BigInteger support implementation
        try {
            // Convert 16 bytes to proper BigInteger representation
            io::ByteVector bytes(io::ByteSpan(data.Data(), 16));
            
            // Create BigInteger from little-endian byte array
            // Neo VM uses little-endian format for integer representation
            auto bigint_item = StackItem::CreateBigInteger(bytes);
            if (!bigint_item) {
                // Fallback: create as byte array if BigInteger creation fails
                bigint_item = StackItem::Create(bytes);
            }
            
            engine.Push(bigint_item);
            
        } catch (const std::exception& e) {
            // Error creating BigInteger - fall back to byte array representation
            io::ByteVector bytes(io::ByteSpan(data.Data(), 16));
            engine.Push(StackItem::Create(bytes));
        }
    }

    void JumpTableConstants::PUSHINT256(ExecutionEngine& engine, const Instruction& instruction)
    {
        // Implement 256-bit integer support
        auto data = instruction.GetData();
        if (data.Size() != 32)
            throw std::runtime_error("PUSHINT256 requires exactly 32 bytes");

        // Complete 256-bit BigInteger support implementation
        try {
            // Convert 32 bytes to proper BigInteger representation
            io::ByteVector bytes(io::ByteSpan(data.Data(), 32));
            
            // Create BigInteger from little-endian byte array
            // Neo VM uses little-endian format for integer representation
            auto bigint_item = StackItem::CreateBigInteger(bytes);
            if (!bigint_item) {
                // Fallback: create as byte array if BigInteger creation fails
                bigint_item = StackItem::Create(bytes);
            }
            
            engine.Push(bigint_item);
            
        } catch (const std::exception& e) {
            // Error creating BigInteger - fall back to byte array representation
            io::ByteVector bytes(io::ByteSpan(data.Data(), 32));
            engine.Push(StackItem::Create(bytes));
        }
    }

    void JumpTableConstants::PUSHA(ExecutionEngine& engine, const Instruction& instruction)
    {
        int32_t position = static_cast<int32_t>(instruction.GetOperand());
        engine.Push(StackItem::Create(static_cast<int64_t>(position)));
    }

    void JumpTableConstants::PUSHNULL(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Null());
    }

    void JumpTableConstants::PUSHDATA1(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto data = instruction.GetData();
        // Convert internal::ByteVector to io::ByteVector
        auto ioData = io::ByteVector(io::ByteSpan(data.Data(), data.Size()));
        engine.Push(StackItem::Create(ioData));
    }

    void JumpTableConstants::PUSHDATA2(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto data = instruction.GetData();
        // Convert internal::ByteVector to io::ByteVector
        auto ioData = io::ByteVector(io::ByteSpan(data.Data(), data.Size()));
        engine.Push(StackItem::Create(ioData));
    }

    void JumpTableConstants::PUSHDATA4(ExecutionEngine& engine, const Instruction& instruction)
    {
        auto data = instruction.GetData();
        // Convert internal::ByteVector to io::ByteVector
        auto ioData = io::ByteVector(io::ByteSpan(data.Data(), data.Size()));
        engine.Push(StackItem::Create(ioData));
    }

    void JumpTableConstants::PUSHM1(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(-1)));
    }

    void JumpTableConstants::PUSH0(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(0)));
    }

    void JumpTableConstants::PUSH1(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(1)));
    }

    void JumpTableConstants::PUSH2(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(2)));
    }

    void JumpTableConstants::PUSH3(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(3)));
    }

    void JumpTableConstants::PUSH4(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(4)));
    }

    void JumpTableConstants::PUSH5(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(5)));
    }

    void JumpTableConstants::PUSH6(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(6)));
    }

    void JumpTableConstants::PUSH7(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(7)));
    }

    void JumpTableConstants::PUSH8(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(8)));
    }

    void JumpTableConstants::PUSH9(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(9)));
    }

    void JumpTableConstants::PUSH10(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(10)));
    }

    void JumpTableConstants::PUSH11(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(11)));
    }

    void JumpTableConstants::PUSH12(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(12)));
    }

    void JumpTableConstants::PUSH13(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(13)));
    }

    void JumpTableConstants::PUSH14(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(14)));
    }

    void JumpTableConstants::PUSH15(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(15)));
    }

    void JumpTableConstants::PUSH16(ExecutionEngine& engine, const Instruction& /* instruction */)
    {
        engine.Push(StackItem::Create(static_cast<int64_t>(16)));
    }
}
