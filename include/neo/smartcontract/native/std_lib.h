#pragma once

#include <neo/smartcontract/native/native_contract.h>
#include <memory>
#include <string>

namespace neo::smartcontract::native
{
    /**
     * @brief Represents the standard library native contract.
     */
    class StdLib : public NativeContract
    {
        // Friend classes for testing
        friend class NativeContractTest;
        friend class StdLibTest;
        
    public:
        /**
         * @brief The contract ID (matches Neo C# implementation).
         */
        static constexpr int32_t ID = 1;

        /**
         * @brief The contract name.
         */
        static constexpr const char* NAME = "StdLib";

        /**
         * @brief Constructs a StdLib.
         */
        StdLib();

    protected:
        /**
         * @brief Initializes the contract.
         */
        void Initialize() override;

    private:
        /**
         * @brief Handles the serialize method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnSerialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the deserialize method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnDeserialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the jsonSerialize method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnJsonSerialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the jsonDeserialize method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnJsonDeserialize(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the itoa method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnItoa(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the atoi method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnAtoi(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base64Encode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase64Encode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base64Decode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase64Decode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base58Encode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase58Encode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base58Decode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase58Decode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base58CheckEncode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase58CheckEncode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base58CheckDecode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase58CheckDecode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base64UrlEncode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase64UrlEncode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the base64UrlDecode method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnBase64UrlDecode(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the strLen method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnStrLen(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the memoryCompare method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnMemoryCompare(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the memoryCopy method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnMemoryCopy(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the memorySearch method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnMemorySearch(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the stringCompare method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnStringCompare(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);
    };
}
