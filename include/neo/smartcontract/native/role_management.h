#pragma once

#include <cstdint>
#include <memory>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/native/native_contract.h>
#include <string>
#include <vector>

namespace neo::smartcontract::native
{
/**
 * @brief Represents the roles in the NEO system.
 */
enum class Role : uint8_t
{
    /**
     * @brief State validator role.
     */
    StateValidator = 4,

    /**
     * @brief Oracle role.
     */
    Oracle = 8,

    /**
     * @brief NeoFS Alphabet Node role.
     */
    NeoFSAlphabetNode = 16,

    /**
     * @brief P2P Notary role.
     */
    P2PNotary = 32
};

/**
 * @brief Represents a list of nodes.
 */
class NodeList
{
  public:
    /**
     * @brief Constructs an empty NodeList.
     */
    NodeList() = default;

    /**
     * @brief Adds a node to the list.
     * @param node The node to add.
     */
    void Add(const cryptography::ecc::ECPoint& node);

    /**
     * @brief Adds multiple nodes to the list.
     * @param nodes The nodes to add.
     */
    void AddRange(const std::vector<cryptography::ecc::ECPoint>& nodes);

    /**
     * @brief Sorts the nodes in the list.
     */
    void Sort();

    /**
     * @brief Converts the list to an array.
     * @return The array of nodes.
     */
    std::vector<cryptography::ecc::ECPoint> ToArray() const;

    /**
     * @brief Serializes the list to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const;

    /**
     * @brief Deserializes the list from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader);

  private:
    std::vector<cryptography::ecc::ECPoint> nodes_;
};

/**
 * @brief Represents the role management native contract.
 */
class RoleManagement : public NativeContract
{
    // Friend classes for testing
    friend class NativeContractTest;
    friend class RoleManagementTest;

  public:
    /**
     * @brief The contract ID.
     */
    static constexpr int32_t ID = -8;

    /**
     * @brief The contract name.
     */
    static constexpr const char* NAME = "RoleManagement";

    /**
     * @brief The storage prefix for roles.
     */
    static constexpr uint8_t PREFIX_ROLE = 33;

    /**
     * @brief Role state constants for committee.
     */
    static constexpr Role ROLE_STATE_COMMITTEE = Role::StateValidator;

    /**
     * @brief Role state constants for validators.
     */
    static constexpr Role ROLE_STATE_VALIDATOR = Role::StateValidator;

    /**
     * @brief Constructs a RoleManagement.
     */
    RoleManagement();

    /**
     * @brief Gets the instance.
     * @return The instance.
     */
    static std::shared_ptr<RoleManagement> GetInstance();

    /**
     * @brief Gets the designated by role.
     * @param snapshot The snapshot.
     * @param role The role.
     * @param index The index of the block to be queried.
     * @return The designated by role.
     */
    std::vector<cryptography::ecc::ECPoint> GetDesignatedByRole(std::shared_ptr<persistence::StoreView> snapshot,
                                                                Role role, uint32_t index) const;

    /**
     * @brief Designates nodes for a role.
     * @param engine The engine.
     * @param role The role.
     * @param nodes The nodes.
     */
    void DesignateAsRole(ApplicationEngine& engine, Role role, const std::vector<cryptography::ecc::ECPoint>& nodes);

    /**
     * @brief Creates a storage key for a role.
     * @param role The role.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t role) const;

    /**
     * @brief Creates a storage key for a role and index.
     * @param role The role.
     * @param index The index.
     * @return The storage key.
     */
    persistence::StorageKey CreateStorageKey(uint8_t role, uint32_t index) const;

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
     * @brief Handles the getDesignatedByRole method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnGetDesignatedByRole(ApplicationEngine& engine,
                                                         const std::vector<std::shared_ptr<vm::StackItem>>& args);

    /**
     * @brief Handles the designateAsRole method.
     * @param engine The engine.
     * @param args The arguments.
     * @return The result.
     */
    std::shared_ptr<vm::StackItem> OnDesignateAsRole(ApplicationEngine& engine,
                                                     const std::vector<std::shared_ptr<vm::StackItem>>& args);
};
}  // namespace neo::smartcontract::native
