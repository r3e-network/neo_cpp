#include <neo/wallets/asset_descriptor.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/vm/script_builder.h>
#include <neo/vm/op_code.h>
#include <stdexcept>

namespace neo::wallets
{
    AssetDescriptor::AssetDescriptor(const persistence::DataCache& snapshot, const config::ProtocolSettings& settings, const io::UInt160& assetId)
        : assetId_(assetId), decimals_(0)
    {
        // Get the contract
        auto contract = smartcontract::native::ContractManagement::GetContract(snapshot, assetId);
        if (!contract)
            throw std::invalid_argument("Invalid asset id");

        // Get the contract name
        assetName_ = contract->GetManifest().GetName();

        // Build script to get decimals and symbol
        vm::ScriptBuilder sb;
        sb.EmitDynamicCall(assetId, "decimals", smartcontract::CallFlags::ReadOnly);
        sb.EmitDynamicCall(assetId, "symbol", smartcontract::CallFlags::ReadOnly);
        auto script = sb.ToArray();

        // Run the script
        smartcontract::ApplicationEngine engine(smartcontract::TriggerType::Application, nullptr, snapshot, 0, true, settings);
        engine.LoadScript(script);
        
        // Execute the script
        if (engine.Execute() != vm::VMState::HALT)
            throw std::invalid_argument("Failed to execute script");

        // Get the results
        if (engine.GetResultStack().size() != 2)
            throw std::invalid_argument("Invalid result stack size");

        // Get the symbol
        symbol_ = engine.GetResultStack().top()->GetString();
        engine.GetResultStack().pop();

        // Get the decimals
        decimals_ = static_cast<uint8_t>(engine.GetResultStack().top()->GetInteger());
        engine.GetResultStack().pop();
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
