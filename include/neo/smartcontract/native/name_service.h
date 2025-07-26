#pragma once

#include <neo/smartcontract/native/native_contract.h>
#include <neo/io/uint160.h>
#include <memory>
#include <string>
#include <tuple>

namespace neo::smartcontract::native
{
    /**
     * @brief Represents the name service native contract.
     */
    class NameService : public NativeContract
    {
        // Friend classes for testing
        friend class NativeContractTest;
        friend class NameServiceTest;
        
    public:
        /**
         * @brief The contract ID.
         */
        static constexpr int32_t ID = -11;

        /**
         * @brief The contract name.
         */
        static constexpr const char* NAME = "NameService";

        /**
         * @brief The storage prefix for names.
         */
        static constexpr uint8_t PREFIX_NAME = 1;

        /**
         * @brief The storage prefix for price.
         */
        static constexpr uint8_t PREFIX_PRICE = 0;

        /**
         * @brief The storage prefix for records.
         */
        static constexpr uint8_t PREFIX_RECORD = 2;

        /**
         * @brief The default price.
         */
        static constexpr int64_t DEFAULT_PRICE = 1000LL * 100000000LL; // 1000 GAS

        /**
         * @brief The maximum name length.
         */
        static constexpr size_t MAX_NAME_LENGTH = 255;

        /**
         * @brief The maximum record size.
         */
        static constexpr size_t MAX_RECORD_SIZE = 65535; // 64KB max record size

        /**
         * @brief The registration duration in blocks (approximately 1 year).
         */
        static constexpr uint64_t REGISTRATION_DURATION = 365 * 24 * 60 * 60 / 15;

        /**
         * @brief Constructs a NameService.
         */
        NameService();

        /**
         * @brief Gets the instance.
         * @return The instance.
         */
        static std::shared_ptr<NameService> GetInstance();

        /**
         * @brief Gets the price.
         * @param snapshot The snapshot.
         * @return The price.
         */
        int64_t GetPrice(std::shared_ptr<persistence::StoreView> snapshot) const;

        /**
         * @brief Gets a name.
         * @param snapshot The snapshot.
         * @param name The name.
         * @return The name data.
         */
        std::tuple<io::UInt160, uint64_t> GetName(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name) const;

        /**
         * @brief Checks if a name is available.
         * @param snapshot The snapshot.
         * @param name The name.
         * @return True if the name is available, false otherwise.
         */
        bool IsAvailable(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name) const;

        /**
         * @brief Gets a record.
         * @param snapshot The snapshot.
         * @param name The name.
         * @param type The record type.
         * @return The record value.
         */
        std::string GetRecord(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name, const std::string& type) const;

        /**
         * @brief Sets a record.
         * @param snapshot The snapshot.
         * @param name The name.
         * @param type The record type.
         * @param value The record value.
         */
        void SetRecord(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name, const std::string& type, const std::string& value);

        /**
         * @brief Deletes a record.
         * @param snapshot The snapshot.
         * @param name The name.
         * @param type The record type.
         */
        void DeleteRecord(std::shared_ptr<persistence::DataCache> snapshot, const std::string& name, const std::string& type);

        /**
         * @brief Checks if the caller is a committee member.
         * @param engine The engine.
         * @return True if the caller is a committee member, false otherwise.
         */
        bool CheckCommittee(ApplicationEngine& engine) const;

        /**
         * @brief Initializes the contract.
         * @param engine The engine.
         * @param hardfork The hardfork version.
         * @return True if successful, false otherwise.
         */
        bool InitializeContract(ApplicationEngine& engine, uint32_t hardfork);

        /**
         * @brief Handles the OnPersist event.
         * @param engine The engine.
         * @return True if successful, false otherwise.
         */
        bool OnPersist(ApplicationEngine& engine);

        /**
         * @brief Handles the PostPersist event.
         * @param engine The engine.
         * @return True if successful, false otherwise.
         */
        bool PostPersist(ApplicationEngine& engine);

    protected:
        /**
         * @brief Initializes the contract.
         */
        void Initialize() override;

    private:
        /**
         * @brief Handles the getPrice method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the setPrice method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnSetPrice(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the isAvailable method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnIsAvailable(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the register method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnRegister(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the renew method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnRenew(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the transfer method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnTransfer(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the delete method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnDelete(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the resolve method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnResolve(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the getOwner method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetOwner(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the getExpiration method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetExpiration(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the getRecord method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnGetRecord(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the setRecord method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnSetRecord(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Handles the deleteRecord method.
         * @param engine The engine.
         * @param args The arguments.
         * @return The result.
         */
        std::shared_ptr<vm::StackItem> OnDeleteRecord(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args);

        /**
         * @brief Validates a name.
         * @param name The name.
         * @return True if the name is valid, false otherwise.
         */
        bool ValidateName(const std::string& name) const;

        /**
         * @brief Validates a record type.
         * @param type The record type.
         * @return True if the record type is valid, false otherwise.
         */
        bool ValidateRecordType(const std::string& type) const;
    };
}
