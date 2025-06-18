#pragma once

#include <neo/smartcontract/contract.h>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/json_writer.h>
#include <neo/io/json_reader.h>
#include <vector>
#include <string>

namespace neo::wallets
{
    /**
     * @brief Represents a verification contract in a wallet.
     */
    class VerificationContract : public io::IJsonSerializable
    {
    public:
        /**
         * @brief Constructs an empty VerificationContract.
         */
        VerificationContract();

        /**
         * @brief Constructs a VerificationContract with the specified contract.
         * @param contract The contract.
         */
        explicit VerificationContract(const smartcontract::Contract& contract);

        /**
         * @brief Constructs a VerificationContract with the specified public key.
         * @param publicKey The public key.
         */
        explicit VerificationContract(const cryptography::ecc::ECPoint& publicKey);

        /**
         * @brief Constructs a VerificationContract with the specified public keys and signature count.
         * @param publicKeys The public keys.
         * @param m The minimum number of signatures required.
         */
        VerificationContract(const std::vector<cryptography::ecc::ECPoint>& publicKeys, int m);

        /**
         * @brief Gets the contract.
         * @return The contract.
         */
        const smartcontract::Contract& GetContract() const;

        /**
         * @brief Sets the contract.
         * @param contract The contract.
         */
        void SetContract(const smartcontract::Contract& contract);

        /**
         * @brief Gets the script hash of the contract.
         * @return The script hash.
         */
        io::UInt160 GetScriptHash() const;

        /**
         * @brief Gets the public keys.
         * @return The public keys.
         */
        const std::vector<cryptography::ecc::ECPoint>& GetPublicKeys() const;

        /**
         * @brief Sets the public keys.
         * @param publicKeys The public keys.
         */
        void SetPublicKeys(const std::vector<cryptography::ecc::ECPoint>& publicKeys);

        /**
         * @brief Gets the parameter names.
         * @return The parameter names.
         */
        const std::vector<std::string>& GetParameterNames() const;

        /**
         * @brief Sets the parameter names.
         * @param parameterNames The parameter names.
         */
        void SetParameterNames(const std::vector<std::string>& parameterNames);

        /**
         * @brief Gets the minimum number of signatures required.
         * @return The minimum number of signatures required.
         */
        int GetM() const;

        /**
         * @brief Sets the minimum number of signatures required.
         * @param m The minimum number of signatures required.
         */
        void SetM(int m);

        /**
         * @brief Determines whether the contract is a signature contract.
         * @return True if the contract is a signature contract, false otherwise.
         */
        bool IsSignatureContract() const;

        /**
         * @brief Determines whether the contract is a multi-signature contract.
         * @return True if the contract is a multi-signature contract, false otherwise.
         */
        bool IsMultiSigContract() const;

        /**
         * @brief Serializes the VerificationContract to a JSON writer.
         * @param writer The JSON writer.
         */
        void SerializeJson(io::JsonWriter& writer) const override;

        /**
         * @brief Deserializes the VerificationContract from a JSON reader.
         * @param reader The JSON reader.
         */
        void DeserializeJson(const io::JsonReader& reader) override;

    private:
        smartcontract::Contract contract_;
        std::vector<cryptography::ecc::ECPoint> publicKeys_;
        std::vector<std::string> parameterNames_;
        int m_;
    };
}
