#include <gtest/gtest.h>
#include <neo/persistence/memory_store.h>
// #include <neo/persistence/store_provider.h>  // File not found
#include <neo/smartcontract/application_engine.h>

using namespace neo::smartcontract;
using namespace neo::persistence;

TEST(SmartContractIntegrationTest, TestApplicationEngineInitialization)
{
    // Create store and snapshot
    auto store = std::make_shared<MemoryStore>();
    auto snapshot = store->GetSnapshot();

    // Create application engine with DataCache
    auto dataCache = std::make_shared<persistence::DataCache>();
    auto engine = std::make_unique<ApplicationEngine>(TriggerType::Application, nullptr, dataCache);

    // Check initial state
    EXPECT_EQ(engine->GetTrigger(), TriggerType::Application);
    EXPECT_EQ(engine->GetGasConsumed(), 0);
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
