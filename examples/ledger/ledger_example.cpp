#include <iostream>
#include <neo/ledger/block.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/ledger/transaction.h>

using namespace neo::ledger;

int main()
{
    // Create a block
    Block block;
    block.SetVersion(0);
    neo::io::UInt256 zero256(std::vector<uint8_t>(32, 0));
    block.SetPreviousHash(zero256);
    block.SetMerkleRoot(zero256);
    block.SetTimestamp(0);
    block.SetIndex(0);
    neo::io::UInt160 zero160(std::vector<uint8_t>(20, 0));
    block.SetNextConsensus(zero160);

    // Print the block
    std::cout << "Block version: " << block.GetVersion() << std::endl;
    std::cout << "Block index: " << block.GetIndex() << std::endl;

    // Create a transaction
    Transaction tx;
    tx.SetVersion(0);
    tx.SetNonce(0);
    tx.SetSystemFee(0);
    tx.SetNetworkFee(0);
    tx.SetValidUntilBlock(0);

    // Print the transaction
    std::cout << "Transaction version: " << tx.GetVersion() << std::endl;
    std::cout << "Transaction nonce: " << tx.GetNonce() << std::endl;

    return 0;
}
