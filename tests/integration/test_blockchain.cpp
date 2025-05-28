#include <gtest/gtest.h>
#include <neo/ledger/blockchain.h>
#include <neo/persistence/memory_store.h>
#include <neo/persistence/store_provider.h>

using namespace neo::ledger;
using namespace neo::persistence;

TEST(BlockchainIntegrationTest, TestBlockchainInitialization) {
    // Create store provider
    auto store = std::make_shared<MemoryStore>();
    auto storeProvider = std::make_shared<StoreProvider>(store);
    
    // Create blockchain
    auto blockchain = std::make_shared<Blockchain>(storeProvider);
    
    // Check initial state
    EXPECT_EQ(blockchain->GetHeight(), 0);
    EXPECT_EQ(blockchain->GetHeaderHeight(), 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
