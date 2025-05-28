#pragma once

#include <neo/smartcontract/native/native_contract.h>
#include <memory>
#include <string>

namespace neo::smartcontract::native
{
    /**
     * @brief Represents the crypto library native contract.
     */
    class CryptoLib : public NativeContract
    {
    public:
        /**
         * @brief The contract ID.
         */
        static constexpr uint32_t ID = 4;

        /**
         * @brief The contract name.
         */
        static constexpr const char* NAME = "CryptoLib";

        /**
         * @brief Constructs a CryptoLib.
         */
        CryptoLib();

    protected:
        /**
         * @brief Initializes the contract.
         */
        void Initialize() override;

    private:
        /**
         * @brief Handles the sha256 method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnSha256(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the ripemd160 method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnRipemd160(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the hash160 method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnHash160(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the hash256 method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnHash256(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the verifySignature method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnVerifySignature(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the verifyWithECDsa method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnVerifyWithECDsa(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the bls12381Serialize method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBls12381Serialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the bls12381Deserialize method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBls12381Deserialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the bls12381Equal method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBls12381Equal(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the bls12381Add method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBls12381Add(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the bls12381Mul method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBls12381Mul(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the bls12381Pairing method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBls12381Pairing(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
    };
}
