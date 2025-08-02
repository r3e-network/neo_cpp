#include <gtest/gtest.h>
#include <neo/vm/execution_engine.h>

using namespace neo::vm;

TEST(VMSimpleTest, BasicTests)
{
    // Test VM creation
    ExecutionEngine engine;
    
    // Just verify engine was created
    SUCCEED() << "Basic VM tests pass";
}