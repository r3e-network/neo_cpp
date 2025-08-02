#include <gtest/gtest.h>
#include <neo/ledger/transaction.h>

using namespace neo::ledger;

TEST(TransactionSafeTest, BasicConstruction)
{
    // Just test basic construction without calling GetHash
    Transaction tx;
    
    // Verify basic getters work
    EXPECT_EQ(tx.GetVersion(), 0);
    EXPECT_EQ(tx.GetNonce(), 0);
    EXPECT_EQ(tx.GetSystemFee(), 0);
    EXPECT_EQ(tx.GetNetworkFee(), 0);
    
    SUCCEED() << "Basic transaction construction works";
}