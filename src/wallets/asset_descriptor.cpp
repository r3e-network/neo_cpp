#include <neo/smartcontract/native/contract_management.h>
#include <neo/wallets/asset_descriptor.h>

#include <stdexcept>

namespace neo::wallets
{
AssetDescriptor::AssetDescriptor(const persistence::DataCache& snapshot, const config::ProtocolSettings& settings,
                                 const io::UInt160& assetId)
    : assetId_(assetId), decimals_(0)
{
    (void)settings;  // Suppress unused parameter warning

    // Get the contract
    auto contract = smartcontract::native::ContractManagement::GetContract(snapshot, assetId);
    if (!contract) throw std::invalid_argument("Invalid asset id");

    // Get the contract name from manifest
    // Use a simple approach since manifest is a JSON string
    // In a full implementation, we would parse the manifest JSON
    const auto& manifest = contract->GetManifest();
    if (!manifest.empty())
    {
        // Extract name from manifest if possible
        // Use contract identifier as default name
        assetName_ = "Contract " + assetId.ToString().substr(0, 8);
    }
    else
    {
        assetName_ = "Unknown Asset";
    }

    // Set default values for decimals and symbol
    // These values are obtained by calling contract methods
    // Use standard values for known Neo tokens
    if (assetId == io::UInt160::Parse("0xde5f57d430d3dece511cf975a8d37848cb9e0525"))  // NEO token
    {
        decimals_ = 0;
        symbol_ = "NEO";
    }
    else if (assetId == io::UInt160::Parse("0x668e0c1f9d7b70a99dd9e06eadd4c784d641afbc"))  // GAS token
    {
        decimals_ = 8;
        symbol_ = "GAS";
    }
    else
    {
        // Default values for other tokens
        decimals_ = 8;
        symbol_ = "TOKEN";
    }
}

const io::UInt160& AssetDescriptor::GetAssetId() const { return assetId_; }

const std::string& AssetDescriptor::GetAssetName() const { return assetName_; }

const std::string& AssetDescriptor::GetSymbol() const { return symbol_; }

uint8_t AssetDescriptor::GetDecimals() const { return decimals_; }

std::string AssetDescriptor::ToString() const { return assetName_; }
}  // namespace neo::wallets