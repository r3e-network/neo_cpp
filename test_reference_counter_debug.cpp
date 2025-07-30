#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/debugger.h>
#include <neo/vm/opcode.h>

using namespace neo::vm;
using namespace neo::io;

int main() {
    ScriptBuilder sb;
    uint8_t slotCount = 1;
    sb.Emit(OpCode::INITSSLOT, ByteSpan(&slotCount, 1));
    sb.Emit(OpCode::RET);
    
    ExecutionEngine engine;
    Debugger debugger(engine);
    
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i) {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);
    
    std::cout << "Before INITSSLOT: Count = " << engine.GetReferenceCounter()->Count() << std::endl;
    
    auto state = debugger.StepInto();
    std::cout << "After INITSSLOT: Count = " << engine.GetReferenceCounter()->Count() << std::endl;
    std::cout << "VM State: " << (state == VMState::Break ? "Break" : "Other") << std::endl;
    
    return 0;
}