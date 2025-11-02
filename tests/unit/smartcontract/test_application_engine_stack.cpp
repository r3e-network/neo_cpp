#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_cache.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>

using namespace neo::smartcontract;
using namespace neo::persistence;
using namespace neo::vm;
using namespace neo::io;

TEST(ApplicationEngineStackTest, ReturnsResultStackAndScript)
{
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = std::make_shared<StoreCache>(*store);

    ApplicationEngine engine(TriggerType::Application, nullptr, snapshot);

    ScriptBuilder builder;
    builder.EmitPush(true);
    builder.Emit(OpCode::RET);
    auto scriptBytes = builder.ToArray();
    std::vector<uint8_t> script(scriptBytes.begin(), scriptBytes.end());
    engine.LoadScript(script, 0);
    const auto state = engine.Execute();

    ASSERT_EQ(state, VMState::Halt);

    const auto resultStack = engine.GetResultStack();
    ASSERT_EQ(resultStack.size(), 1);
    EXPECT_TRUE(resultStack[0]->GetBoolean());

    const auto returnedScript = engine.GetScript();
    EXPECT_EQ(returnedScript.Size(), scriptBytes.Size());
    EXPECT_TRUE(std::equal(returnedScript.begin(), returnedScript.end(), scriptBytes.begin()));
}
