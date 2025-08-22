/**
 * @file blockchain_import_commands.cpp
 * @brief Blockchain import commands for fast sync functionality
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#include <neo/cli/blockchain_import_commands.h>
#include <neo/cli/cli.h>
#include <neo/core/exceptions.h>
#include <neo/core/logging.h>
#include <neo/io/binary_reader.h>
#include <neo/ledger/blockchain.h>
#include <neo/ledger/block.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>
#include <ziplib/ZipFile.h>

namespace neo::cli
{

/**
 * @brief Blockchain import utility class
 * Implements fast sync functionality compatible with C# Neo node
 */
class BlockchainImporter
{
public:
    BlockchainImporter(std::shared_ptr<ledger::Blockchain> blockchain)
        : blockchain_(blockchain) {}

    /**
     * @brief Import blocks from .acc file format (C# compatible)
     * @param file_path Path to .acc or .acc.zip file
     * @param verify Whether to verify blocks during import
     * @return Number of blocks imported
     */
    uint32_t ImportFromAccFile(const std::string& file_path, bool verify = true)
    {
        LOG_INFO("Starting blockchain import from: {}", file_path);
        
        if (!std::filesystem::exists(file_path)) {
            throw core::BlockchainException("Import file not found: " + file_path);
        }
        
        auto file_size = std::filesystem::file_size(file_path);
        LOG_INFO("Import file size: {} bytes ({:.2f} MB)", file_size, file_size / 1024.0 / 1024.0);
        
        if (file_path.ends_with(".acc.zip")) {
            return ImportFromCompressedAcc(file_path, verify);
        } else if (file_path.ends_with(".acc")) {
            return ImportFromUncompressedAcc(file_path, verify);
        } else {
            throw core::BlockchainException("Unsupported file format. Use .acc or .acc.zip");
        }
    }

private:
    /**
     * @brief Import from compressed .acc.zip file
     */
    uint32_t ImportFromCompressedAcc(const std::string& zip_path, bool verify)
    {
        LOG_INFO("Extracting compressed blockchain data...");
        
        // Extract the .acc file from the zip
        std::string temp_acc_path = "/tmp/chain_import.acc";
        
        try {
            // Use basic zip extraction (production would use libzip or similar)
            std::string extract_cmd = "cd /tmp && unzip -o \"" + zip_path + "\" 2>/dev/null";
            int result = std::system(extract_cmd.c_str());
            
            if (result != 0) {
                throw core::BlockchainException("Failed to extract zip file");
            }
            
            // Find the extracted .acc file
            for (const auto& entry : std::filesystem::directory_iterator("/tmp")) {
                if (entry.path().extension() == ".acc") {
                    temp_acc_path = entry.path().string();
                    break;
                }
            }
            
            if (!std::filesystem::exists(temp_acc_path)) {
                throw core::BlockchainException("No .acc file found in zip archive");
            }
            
            LOG_INFO("Extracted to: {}", temp_acc_path);
            
            // Import from the extracted file
            uint32_t imported = ImportFromUncompressedAcc(temp_acc_path, verify);
            
            // Cleanup
            std::filesystem::remove(temp_acc_path);
            
            return imported;
            
        } catch (const std::exception& e) {
            // Cleanup on error
            if (std::filesystem::exists(temp_acc_path)) {
                std::filesystem::remove(temp_acc_path);
            }
            throw;
        }
    }
    
    /**
     * @brief Import from uncompressed .acc file
     */
    uint32_t ImportFromUncompressedAcc(const std::string& acc_path, bool verify)
    {
        std::ifstream file(acc_path, std::ios::binary);
        if (!file.is_open()) {
            throw core::BlockchainException("Cannot open import file: " + acc_path);
        }
        
        io::BinaryReader reader(file);
        
        // Read header information (matching C# format exactly)
        uint32_t start_index = 0;
        uint32_t block_count = 0;
        
        try {
            // The .acc format starts with block count (C# implementation)
            block_count = reader.ReadUInt32();
            
            LOG_INFO("Import file contains {} blocks", block_count);
            
            if (block_count == 0) {
                LOG_WARNING("No blocks to import");
                return 0;
            }
            
            // Get current blockchain height
            uint32_t current_height = blockchain_->GetHeight();
            LOG_INFO("Current blockchain height: {}", current_height);
            
            // Import blocks in batches for better performance
            const uint32_t BATCH_SIZE = 1000;
            uint32_t imported_count = 0;
            uint32_t skipped_count = 0;
            
            ledger::ImportData import_data;
            import_data.verify = verify;
            
            for (uint32_t i = 0; i < block_count; i++) {
                // Read block size
                int32_t block_size = reader.ReadInt32();
                
                if (block_size <= 0 || block_size > 10 * 1024 * 1024) { // 10MB max
                    throw core::BlockchainException("Invalid block size: " + std::to_string(block_size));
                }
                
                // Read block data
                auto block_data = reader.ReadBytes(block_size);
                
                if (block_data.size() != static_cast<size_t>(block_size)) {
                    throw core::BlockchainException("Failed to read complete block data");
                }
                
                // Deserialize block
                auto block = DeserializeBlock(block_data);
                if (!block) {
                    LOG_ERROR("Failed to deserialize block at index {}", i);
                    continue;
                }
                
                // Skip blocks we already have
                if (block->GetIndex() <= current_height) {
                    skipped_count++;
                    continue;
                }
                
                // Add to import batch
                import_data.blocks.push_back(block);
                
                // Import batch when full
                if (import_data.blocks.size() >= BATCH_SIZE) {
                    if (blockchain_->ImportBlocks(import_data)) {
                        imported_count += import_data.blocks.size();
                        LOG_INFO("Imported batch: {} blocks (total: {})", 
                                import_data.blocks.size(), imported_count);
                    } else {
                        throw core::BlockchainException("Failed to import block batch");
                    }
                    
                    import_data.blocks.clear();
                }
                
                // Progress reporting
                if ((i + 1) % 10000 == 0) {
                    LOG_INFO("Progress: {}/{} blocks processed", i + 1, block_count);
                }
            }
            
            // Import remaining blocks
            if (!import_data.blocks.empty()) {
                if (blockchain_->ImportBlocks(import_data)) {
                    imported_count += import_data.blocks.size();
                    LOG_INFO("Imported final batch: {} blocks", import_data.blocks.size());
                } else {
                    throw core::BlockchainException("Failed to import final block batch");
                }
            }
            
            LOG_INFO("Import completed successfully!");
            LOG_INFO("Total blocks imported: {}", imported_count);
            LOG_INFO("Blocks skipped (already exists): {}", skipped_count);
            LOG_INFO("Final blockchain height: {}", blockchain_->GetHeight());
            
            return imported_count;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Import failed: {}", e.what());
            throw core::BlockchainException("Blockchain import failed: " + std::string(e.what()));
        }
    }
    
    /**
     * @brief Deserialize block from binary data (C# AsSerializable<Block> equivalent)
     */
    std::shared_ptr<ledger::Block> DeserializeBlock(const io::ByteVector& data)
    {
        try {
            std::istringstream stream(std::string(data.begin(), data.end()));
            io::BinaryReader reader(stream);
            
            // Create new block and deserialize
            auto block = std::make_shared<ledger::Block>();
            
            // Read block header
            block->SetVersion(reader.ReadByte());
            block->SetPreviousHash(reader.ReadUInt256());
            block->SetMerkleRoot(reader.ReadUInt256());
            block->SetTimestamp(reader.ReadUInt64());
            block->SetIndex(reader.ReadUInt32());
            
            // Read primary field (consensus data)
            auto primary = reader.ReadByte();
            block->SetPrimary(primary);
            
            // Read next consensus address
            auto next_consensus = reader.ReadUInt160();
            block->SetNextConsensus(next_consensus);
            
            // Read witnesses
            auto witness_count = reader.ReadVarInt();
            std::vector<std::shared_ptr<ledger::Witness>> witnesses;
            
            for (uint64_t i = 0; i < witness_count; i++) {
                auto witness = std::make_shared<ledger::Witness>();
                witness->Deserialize(reader);
                witnesses.push_back(witness);
            }
            block->SetWitnesses(witnesses);
            
            // Read transactions
            auto tx_count = reader.ReadVarInt();
            std::vector<std::shared_ptr<ledger::Transaction>> transactions;
            
            for (uint64_t i = 0; i < tx_count; i++) {
                auto transaction = std::make_shared<ledger::Transaction>();
                transaction->Deserialize(reader);
                transactions.push_back(transaction);
            }
            block->SetTransactions(transactions);
            
            return block;
            
        } catch (const std::exception& e) {
            LOG_ERROR("Block deserialization failed: {}", e.what());
            return nullptr;
        }
    }

    std::shared_ptr<ledger::Blockchain> blockchain_;
};

// CLI command implementation
bool CLI::HandleImportBlockchain(const std::vector<std::string>& args)
{
    if (args.empty()) {
        std::cout << "Usage: import blockchain <file.acc|file.acc.zip> [--no-verify]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  import blockchain chain.0.acc.zip" << std::endl;
        std::cout << "  import blockchain chain.0.acc --no-verify" << std::endl;
        return false;
    }
    
    std::string file_path = args[0];
    bool verify = true;
    
    // Check for --no-verify flag
    if (args.size() > 1 && args[1] == "--no-verify") {
        verify = false;
        LOG_WARNING("Block verification disabled during import");
    }
    
    try {
        auto blockchain = GetBlockchain();
        if (!blockchain) {
            std::cout << "Error: Blockchain not available" << std::endl;
            return false;
        }
        
        BlockchainImporter importer(blockchain);
        
        std::cout << "Starting blockchain import..." << std::endl;
        std::cout << "File: " << file_path << std::endl;
        std::cout << "Verification: " << (verify ? "enabled" : "disabled") << std::endl;
        std::cout << "Current height: " << blockchain->GetHeight() << std::endl;
        
        auto start_time = std::chrono::steady_clock::now();
        
        uint32_t imported = importer.ImportFromAccFile(file_path, verify);
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        
        std::cout << "Import completed successfully!" << std::endl;
        std::cout << "Blocks imported: " << imported << std::endl;
        std::cout << "Time taken: " << duration.count() << " seconds" << std::endl;
        std::cout << "New blockchain height: " << blockchain->GetHeight() << std::endl;
        
        if (imported > 0) {
            double blocks_per_second = static_cast<double>(imported) / duration.count();
            std::cout << "Import rate: " << std::fixed << std::setprecision(2) 
                     << blocks_per_second << " blocks/second" << std::endl;
        }
        
        return true;
        
    } catch (const core::NeoException& e) {
        std::cout << "Import failed: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cout << "Import error: " << e.what() << std::endl;
        return false;
    }
}

}  // namespace neo::cli