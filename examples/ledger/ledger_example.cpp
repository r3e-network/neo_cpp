#include <iostream>
#include <neo/ledger/block.h>
#include <neo/ledger/transaction.h>

using namespace neo::ledger;

int main() {
    // Create a block
    Block block;
    block.SetVersion(0);
    block.SetPrevHash(std::vector<uint8_t>(32, 0));
    block.SetMerkleRoot(std::vector<uint8_t>(32, 0));
    block.SetTimestamp(0);
    block.SetIndex(0);
    block.SetConsensusData(0);
    block.SetNextConsensus(std::vector<uint8_t>(20, 0));
    
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
