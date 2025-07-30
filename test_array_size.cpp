#include <iostream>
#include <neo/vm/execution_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/opcode.h>
#include <neo/vm/execution_engine_limits.h>

using namespace neo::vm;
using namespace neo::io;

int main() {
    ScriptBuilder sb;
    auto size = ExecutionEngineLimits::Default.MaxStackSize - 1;
    std::cout << "Creating array of size: " << size << std::endl;
    
    sb.EmitPush(static_cast<int64_t>(size));
    sb.Emit(OpCode::NEWARRAY);
    
    ExecutionEngine engine;
    auto bytes = sb.ToArray();
    neo::vm::internal::ByteVector internalBytes;
    internalBytes.Reserve(bytes.Size());
    for (size_t i = 0; i < bytes.Size(); ++i) {
        internalBytes.Push(bytes[i]);
    }
    Script script(internalBytes);
    engine.LoadScript(script);
    
    auto state = engine.Execute();
    std::cout << "Execution state: " << (state == VMState::Halt ? "HALT" : "FAULT") << std::endl;
    std::cout << "Reference count: " << engine.GetReferenceCounter()->Count() << std::endl;
    
    return 0;
}