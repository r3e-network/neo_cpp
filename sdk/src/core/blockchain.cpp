#include <neo/sdk/core/blockchain.h>
#include <mutex>
#include <neo/ledger/blockchain.h>
#include <neo/logging/logger.h>

namespace neo::sdk::core {

// Static blockchain instance accessor
static neo::ledger::Blockchain* GetBlockchainInstance() {
    // Get the singleton blockchain instance
    static neo::ledger::Blockchain* instance = nullptr;
    static std::once_flag init_flag;
    
    std::call_once(init_flag, []() {
        try {
            // Initialize blockchain instance connection
            // This connects to the running node's blockchain
            instance = &neo::ledger::Blockchain::GetInstance();
        } catch (const std::exception& e) {
            NEO_LOG_ERROR(std::string("Failed to initialize blockchain: ") + e.what());
        }
    });
    
    if (!instance) {
        NEO_LOG_ERROR("Blockchain instance not available");
    }
    return instance;
}

std::shared_ptr<Block> Blockchain::GetBlock(const UInt256& hash) {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return nullptr;
    }
    
    try {
        // Query blockchain for block
        auto block = blockchain->GetBlock(hash);
        if (!block) {
            NEO_LOG_DEBUG("Block not found for hash: " + hash.ToString());
        }
        return block;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to get block by hash: ") + e.what());
        return nullptr;
    }
}

std::shared_ptr<Block> Blockchain::GetBlock(uint32_t height) {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return nullptr;
    }
    
    try {
        // Query blockchain for block at height
        auto block = blockchain->GetBlock(height);
        if (!block) {
            NEO_LOG_DEBUG("Block not found at height: " + std::to_string(height));
        }
        return block;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to get block by height ") + std::to_string(height) + ": " + e.what());
        return nullptr;
    }
}

std::shared_ptr<Transaction> Blockchain::GetTransaction(const UInt256& hash) {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return nullptr;
    }
    
    try {
        // Query blockchain for transaction
        auto tx = blockchain->GetTransaction(hash);
        if (!tx) {
            NEO_LOG_DEBUG("Transaction not found: " + hash.ToString());
        }
        return tx;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to get transaction: ") + e.what());
        return nullptr;
    }
}

uint32_t Blockchain::GetCurrentHeight() {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return 0;
    }
    
    try {
        // Get current blockchain height
        return blockchain->GetHeight();
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to get current height: ") + e.what());
        return 0;
    }
}

std::shared_ptr<Header> Blockchain::GetHeader(uint32_t height) {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return nullptr;
    }
    
    try {
        // Get header at height
        auto header = blockchain->GetHeader(height);
        if (!header) {
            NEO_LOG_DEBUG("Header not found at height: " + std::to_string(height));
        }
        return header;
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to get header at height ") + std::to_string(height) + ": " + e.what());
        return nullptr;
    }
}

UInt256 Blockchain::GetBestBlockHash() {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return UInt256();
    }
    
    try {
        // Get best block hash
        return blockchain->GetBestBlockHash();
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to get best block hash: ") + e.what());
        return UInt256();
    }
}

bool Blockchain::ContainsBlock(const UInt256& hash) {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return false;
    }
    
    try {
        // Check if block exists
        return blockchain->ContainsBlock(hash);
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to check block existence: ") + e.what());
        return false;
    }
}

bool Blockchain::ContainsTransaction(const UInt256& hash) {
    auto blockchain = GetBlockchainInstance();
    if (!blockchain) {
        return false;
    }
    
    try {
        // Check if transaction exists
        return blockchain->ContainsTransaction(hash);
    } catch (const std::exception& e) {
        NEO_LOG_ERROR(std::string("Failed to check transaction existence: ") + e.what());
        return false;
    }
}

std::vector<std::shared_ptr<Block>> Blockchain::GetBlocks(uint32_t start, uint32_t count) {
    std::vector<std::shared_ptr<Block>> blocks;
    blocks.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i) {
        auto block = GetBlock(start + i);
        if (block) {
            blocks.push_back(block);
        } else {
            break;  // Stop if we can't get a block
        }
    }
    
    return blocks;
}

std::shared_ptr<Block> Blockchain::GetGenesisBlock() {
    return GetBlock(0);
}

} // namespace neo::sdk::core