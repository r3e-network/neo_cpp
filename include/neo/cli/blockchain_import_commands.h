/**
 * @file blockchain_import_commands.h
 * @brief Blockchain import commands for fast sync functionality
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/io/byte_vector.h>
#include <neo/ledger/blockchain.h>

#include <memory>
#include <string>
#include <vector>

namespace neo::cli
{

/**
 * @brief Blockchain import utility for fast sync
 * Compatible with C# Neo node .acc file format
 */
class BlockchainImporter
{
public:
    /**
     * @brief Constructor
     * @param blockchain The blockchain instance to import into
     */
    explicit BlockchainImporter(std::shared_ptr<ledger::Blockchain> blockchain);
    
    /**
     * @brief Import blocks from .acc or .acc.zip file
     * @param file_path Path to the import file
     * @param verify Whether to verify blocks during import
     * @return Number of blocks imported
     */
    uint32_t ImportFromAccFile(const std::string& file_path, bool verify = true);
    
    /**
     * @brief Check if import file is valid
     * @param file_path Path to check
     * @return True if file can be imported
     */
    bool ValidateImportFile(const std::string& file_path);
    
    /**
     * @brief Get import file statistics
     * @param file_path Path to analyze
     * @return Import file information
     */
    struct ImportFileInfo {
        uint32_t block_count;
        uint32_t start_index;
        uint64_t total_size;
        bool is_compressed;
    };
    
    ImportFileInfo AnalyzeImportFile(const std::string& file_path);

private:
    uint32_t ImportFromCompressedAcc(const std::string& zip_path, bool verify);
    uint32_t ImportFromUncompressedAcc(const std::string& acc_path, bool verify);
    std::shared_ptr<ledger::Block> DeserializeBlock(const io::ByteVector& data);
    
    std::shared_ptr<ledger::Blockchain> blockchain_;
};

}  // namespace neo::cli