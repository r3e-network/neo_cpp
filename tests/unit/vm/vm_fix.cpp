// VM stack operation fixes
#include <neo/vm/execution_engine.h>

namespace neo::vm {
    
void ExecutionEngine::Push(const StackItem& item) {
    if (state_ != VMState::HALT) {
        evaluation_stack_.push(item);
    }
}

StackItem ExecutionEngine::Pop() {
    if (evaluation_stack_.empty()) {
        state_ = VMState::FAULT;
        throw std::runtime_error("Stack underflow");
    }
    
    StackItem item = evaluation_stack_.top();
    evaluation_stack_.pop();
    return item;
}

size_t ExecutionEngine::GetStackSize() const {
    return evaluation_stack_.size();
}

}
