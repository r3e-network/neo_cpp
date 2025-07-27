#pragma once

#include <neo/io/uint160.h>
#include <neo/ledger/witness.h>
#include <vector>

namespace neo::network::p2p::payloads
{
/**
 * @brief Interface for verifiable items in the Neo network.
 */
class IVerifiable
{
  public:
    virtual ~IVerifiable() = default;

    /**
     * @brief Gets the script hashes that need to be verified.
     * @return The script hashes for verification.
     */
    virtual std::vector<io::UInt160> GetScriptHashesForVerifying() const = 0;

    /**
     * @brief Gets the witnesses for verification.
     * @return The witnesses.
     */
    virtual const std::vector<ledger::Witness>& GetWitnesses() const = 0;

    /**
     * @brief Sets the witnesses.
     * @param witnesses The witnesses.
     */
    virtual void SetWitnesses(const std::vector<ledger::Witness>& witnesses) = 0;
};
}  // namespace neo::network::p2p::payloads