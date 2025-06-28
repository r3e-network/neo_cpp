#include <neo/ledger/header_cache.h>
#include <neo/ledger/block_header.h>
#include <iostream>

int main()
{
    neo::ledger::HeaderCache cache(10);
    
    // Create first header
    auto header1 = std::make_shared<neo::ledger::BlockHeader>();
    header1->SetIndex(1);
    header1->SetTimestamp(1000);
    header1->SetPrevHash(neo::io::UInt256::Zero());
    header1->SetMerkleRoot(neo::io::UInt256::Zero());
    header1->SetNonce(0);
    header1->SetPrimaryIndex(0);
    header1->SetNextConsensus(neo::io::UInt160::Zero());
    
    std::cout << "Header1 hash: " << header1->GetHash().ToString() << std::endl;
    
    cache.Add(header1);
    std::cout << "After adding header1: size = " << cache.Size() << std::endl;
    
    // Create second header with same index but different timestamp
    auto header2 = std::make_shared<neo::ledger::BlockHeader>();
    header2->SetIndex(1);
    header2->SetTimestamp(9999);
    header2->SetPrevHash(neo::io::UInt256::Zero());
    header2->SetMerkleRoot(neo::io::UInt256::Zero());
    header2->SetNonce(0);
    header2->SetPrimaryIndex(0);
    header2->SetNextConsensus(neo::io::UInt160::Zero());
    
    std::cout << "Header2 hash: " << header2->GetHash().ToString() << std::endl;
    
    cache.Add(header2);
    std::cout << "After adding header2: size = " << cache.Size() << std::endl;
    
    // Check what we can retrieve
    auto byHash1 = cache.Get(header1->GetHash());
    auto byHash2 = cache.Get(header2->GetHash());
    auto byIndex = cache.Get(1);
    
    std::cout << "Get by header1 hash: " << (byHash1 ? "found" : "not found") << std::endl;
    std::cout << "Get by header2 hash: " << (byHash2 ? "found" : "not found") << std::endl;
    std::cout << "Get by index 1: " << (byIndex ? "found" : "not found");
    if (byIndex) {
        std::cout << ", timestamp = " << byIndex->GetTimestamp();
    }
    std::cout << std::endl;
    
    return 0;
}