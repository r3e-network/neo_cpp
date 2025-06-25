#pragma once

#include <neo/io/iserializable.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/uint160.h>
#include <neo/io/uint256.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/smartcontract/contract.h>
#include <neo/persistence/data_cache.h>
#include <neo/network/p2p/payloads/iverifiable.h>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace neo::smartcontract
{
    /**
     * @brief The context used to add witnesses for IVerifiable.
     */
    class ContractParametersContext
    {
    private:
        class ContextItem
        {
        public:
            io::ByteVector script;
            std::vector<ContractParameter> parameters;
            std::map<cryptography::ecc::ECPoint, io::ByteVector> signatures;

            /**
             * @brief Constructs a ContextItem with a contract.
             * @param contract The contract.
             */
            explicit ContextItem(const Contract& contract);

            /**
             * @brief Constructs a ContextItem from a JSON object.
             * @param reader The JSON reader.
             */
            explicit ContextItem(const io::JsonReader& reader);

            /**
             * @brief Converts the item to a JSON object.
             * @param writer The JSON writer.
             */
            void ToJson(io::JsonWriter& writer) const;
        };

    public:
        /**
         * @brief The IVerifiable to add witnesses.
         */
        const network::p2p::payloads::IVerifiable& verifiable;

        /**
         * @brief The snapshot used to read data.
         */
        const persistence::DataCache& snapshotCache;

        /**
         * @brief The magic number of the network.
         */
        const uint32_t network;

        /**
         * @brief Determines whether all witnesses are ready to be added.
         * @return True if all witnesses are ready to be added, false otherwise.
         */
        bool IsCompleted() const;

        /**
         * @brief Gets the script hashes to be verified for the IVerifiable.
         * @return The script hashes to be verified.
         */
        const std::vector<io::UInt160>& GetScriptHashes() const;

        /**
         * @brief Initializes a new instance of the ContractParametersContext class.
         * @param snapshotCache The snapshot used to read data.
         * @param verifiable The IVerifiable to add witnesses.
         * @param network The magic number of the network.
         */
        ContractParametersContext(const persistence::DataCache& snapshotCache, const network::p2p::payloads::IVerifiable& verifiable, uint32_t network);

        /**
         * @brief Adds a parameter to the specified witness script.
         * @param contract The contract contains the script.
         * @param index The index of the parameter.
         * @param parameter The value of the parameter.
         * @return True if the parameter is added successfully, false otherwise.
         */
        bool Add(const Contract& contract, int index, const io::ByteVector& parameter);

        /**
         * @brief Adds parameters to the specified witness script.
         * @param contract The contract contains the script.
         * @param parameters The values of the parameters.
         * @return True if the parameters are added successfully, false otherwise.
         */
        bool Add(const Contract& contract, const std::vector<io::ByteVector>& parameters);

        /**
         * @brief Adds a signature to the specified witness script.
         * @param contract The contract contains the script.
         * @param pubkey The public key for the signature.
         * @param signature The signature.
         * @return True if the signature is added successfully, false otherwise.
         */
        bool AddSignature(const Contract& contract, const cryptography::ecc::ECPoint& pubkey, const io::ByteVector& signature);

        /**
         * @brief Try to add a deployed contract to this context.
         * @param scriptHash The script hash of the contract.
         * @return True if the contract is added successfully, false otherwise.
         */
        bool AddWithScriptHash(const io::UInt160& scriptHash);

        /**
         * @brief Gets the parameter with the specified index from the witness script.
         * @param scriptHash The hash of the witness script.
         * @param index The specified index.
         * @return The parameter with the specified index.
         */
        const ContractParameter* GetParameter(const io::UInt160& scriptHash, int index) const;

        /**
         * @brief Gets the parameters from the witness script.
         * @param scriptHash The hash of the witness script.
         * @return The parameters from the witness script.
         */
        const std::vector<ContractParameter>* GetParameters(const io::UInt160& scriptHash) const;

        /**
         * @brief Gets the signatures from the witness script.
         * @param scriptHash The hash of the witness script.
         * @return The signatures from the witness script.
         */
        const std::map<cryptography::ecc::ECPoint, io::ByteVector>* GetSignatures(const io::UInt160& scriptHash) const;

        /**
         * @brief Gets the witnesses for the IVerifiable.
         * @return The witnesses for the IVerifiable.
         */
        std::vector<ledger::Witness> GetWitnesses() const;

        /**
         * @brief Converts the context from a JSON object.
         * @param reader The JSON reader.
         * @param snapshotCache The snapshot used to read data.
         * @return The converted context.
         */
        static std::unique_ptr<ContractParametersContext> FromJson(const io::JsonReader& reader, const persistence::DataCache& snapshotCache);

        /**
         * @brief Converts the context to a JSON object.
         * @param writer The JSON writer.
         */
        void ToJson(io::JsonWriter& writer) const;

    private:
        std::map<io::UInt160, std::unique_ptr<ContextItem>> contextItems;
        mutable std::vector<io::UInt160> scriptHashes;

        /**
         * @brief Creates a context item for the specified contract.
         * @param contract The contract.
         * @return The created context item.
         */
        ContextItem* CreateItem(const Contract& contract);

        /**
         * @brief Checks if the script is a multi-sig contract.
         * @param script The script to check.
         * @param m The minimum required signatures.
         * @param n The total number of public keys.
         * @param publicKeys The public keys.
         * @return True if the script is a multi-sig contract, false otherwise.
         */
        bool IsMultiSigContract(const io::ByteVector& script, int& m, int& n, std::vector<cryptography::ecc::ECPoint>& publicKeys) const;

        /**
         * @brief Creates a multi-sig witness for the specified contract.
         * @param contract The contract.
         * @return The created witness.
         */
        std::shared_ptr<ledger::Witness> CreateMultiSigWitness(const Contract& contract) const;
    };
}
