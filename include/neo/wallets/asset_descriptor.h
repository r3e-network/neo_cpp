#pragma once

#include <neo/config/protocol_settings.h>
#include <neo/io/uint160.h>
#include <neo/persistence/data_cache.h>

#include <string>

namespace neo::wallets
{
/**
 * @brief Represents the descriptor of an asset.
 */
class AssetDescriptor
{
   public:
    /**
     * @brief Initializes a new instance of the AssetDescriptor class.
     * @param snapshot The snapshot used to read data.
     * @param settings The protocol settings used by the ApplicationEngine.
     * @param assetId The id of the asset.
     * @throws std::invalid_argument if the asset id is invalid.
     */
    AssetDescriptor(const persistence::DataCache& snapshot, const config::ProtocolSettings& settings,
                    const io::UInt160& assetId);

    /**
     * @brief Gets the id of the asset.
     * @return The id of the asset.
     */
    const io::UInt160& GetAssetId() const;

    /**
     * @brief Gets the name of the asset.
     * @return The name of the asset.
     */
    const std::string& GetAssetName() const;

    /**
     * @brief Gets the symbol of the asset.
     * @return The symbol of the asset.
     */
    const std::string& GetSymbol() const;

    /**
     * @brief Gets the number of decimal places of the token.
     * @return The number of decimal places of the token.
     */
    uint8_t GetDecimals() const;

    /**
     * @brief Converts the asset descriptor to a string.
     * @return The name of the asset.
     */
    std::string ToString() const;

   private:
    io::UInt160 assetId_;
    std::string assetName_;
    std::string symbol_;
    uint8_t decimals_;
};
}  // namespace neo::wallets
