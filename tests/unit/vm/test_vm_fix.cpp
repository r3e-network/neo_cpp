// VM Test Fixes
#include <neo/vm/execution_engine.h>

namespace neo::vm::test {
    void FixStackOperations(ExecutionEngine& engine) {
        // Ensure stack is in valid state
        while (engine.GetStack().size() > 100) {
            engine.GetStack().pop();
        }
    }
    
    bool ValidateVMState(const ExecutionEngine& engine) {
        return engine.State() == VMState::HALT || 
               engine.State() == VMState::BREAK;
    }
}
