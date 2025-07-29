#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>
#include <neo/smartcontract/application_engine.h>

using namespace neo::smartcontract;
using namespace neo::persistence;

TEST(SmartContractIntegrationTest, TestApplicationEngineInitialization)
{
    // Create store provider
    auto store = std::make_shared<MemoryStore>();
    auto storeProvider = std::make_shared<StoreProvider>(store);
    auto snapshot = storeProvider->GetSnapshot();

    // Create application engine
    auto engine = std::make_unique<ApplicationEngine>(TriggerType::Application, nullptr, snapshot);

    // Check initial state
    EXPECT_EQ(engine->GetTrigger(), TriggerType::Application);
    EXPECT_EQ(engine->GetGasConsumed(), 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
