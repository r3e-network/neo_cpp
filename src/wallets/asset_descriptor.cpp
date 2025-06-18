#include <neo/wallets/asset_descriptor.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>
#include <stdexcept>

namespace neo::wallets
{
    AssetDescriptor::AssetDescriptor(const persistence::DataCache& snapshot, const config::ProtocolSettings& settings, const io::UInt160& assetId)
        : assetId_(assetId), decimals_(0)
    {
        (void)settings; // Suppress unused parameter warning
        
        // Get the contract
        auto contract = smartcontract::native::ContractManagement::GetContract(snapshot, assetId);
        if (!contract)
            throw std::invalid_argument("Invalid asset id");

        // Get the contract name
        assetName_ = contract->GetManifest();

        // For now, set default values for decimals and symbol
        // TODO: Implement proper contract method calling using System.Contract.Call syscall
        decimals_ = 8;  // Default decimals for most tokens
        symbol_ = "TOKEN";  // Default symbol
        
        // Note: In the full implementation, this would build a script that:
        // 1. Pushes an empty array (no parameters)
        // 2. Pushes the method name ("decimals" or "symbol")
        // 3. Pushes the contract script hash
        // 4. Uses SYSCALL with "System.Contract.Call"
        // 5. Executes the script with ApplicationEngine
    }

    const io::UInt160& AssetDescriptor::GetAssetId() const
    {
        return assetId_;
    }

    const std::string& AssetDescriptor::GetAssetName() const
    {
        return assetName_;
    }

    const std::string& AssetDescriptor::GetSymbol() const
    {
        return symbol_;
    }

    uint8_t AssetDescriptor::GetDecimals() const
    {
        return decimals_;
    }

    std::string AssetDescriptor::ToString() const
    {
        return assetName_;
    }
}
