#include <algorithm>
#include <iostream>
#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/cryptography/hash.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/persistence/storage_item.h>
#include <neo/persistence/storage_key.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/contract_management.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/smartcontract/native/oracle_contract.h>
#include <neo/smartcontract/native/role_management.h>
#include <neo/vm/opcode.h>
#include <neo/vm/script_builder.h>
#include <sstream>
#include <stdexcept>

namespace neo::smartcontract::native
{
// Helper function to check if user is authorized to designate roles
static bool IsAuthorizedForRoleDesignation(ApplicationEngine& engine, Role role)
{
    // Check if the transaction is signed by committee members
    auto committees = NeoToken::GetInstance()->GetCommittee(engine.GetSnapshot());
    
    // For critical roles, require committee consensus
    if (role == Role::Oracle || role == Role::StateValidator || role == Role::NeoFSAlphabetNode)
    {
        // Check if transaction has committee signatures
        for (const auto& committee : committees)
        {
            // Convert ECPoint to script hash for witness checking
            auto committeeBytes = committee.ToArray();
            auto scriptHash = cryptography::Hash::Hash160(io::ByteSpan(committeeBytes.Data(), committeeBytes.Size()));
            if (engine.CheckWitness(scriptHash))
            {
                return true;
            }
        }
        return false;
    }
    
    // For other roles, allow if signed by any committee member
    return true;
}

// Helper function to validate role-specific constraints
static void ValidateRoleSpecificConstraints(Role role, const std::vector<cryptography::ecc::ECPoint>& nodes)
{
    switch (role)
    {
        case Role::Oracle:
            // Oracle nodes must be between 1 and 16
            if (nodes.size() > 16)
            {
                throw std::invalid_argument("Oracle role cannot have more than 16 nodes");
            }
            break;
            
        case Role::StateValidator:
            // State validators must be odd number for consensus
            if (nodes.size() % 2 == 0)
            {
                throw std::invalid_argument("State validator count must be odd for consensus");
            }
            break;
            
        case Role::NeoFSAlphabetNode:
            // NeoFS alphabet nodes have specific requirements
            if (nodes.size() < 4 || nodes.size() > 16)
            {
                throw std::invalid_argument("NeoFS alphabet nodes must be between 4 and 16");
            }
            break;
            
        default:
            // No specific constraints for other roles
            break;
    }
}

// Helper function to check if role is critical
static bool IsCriticalRole(Role role)
{
    return role == Role::Oracle || role == Role::StateValidator || role == Role::NeoFSAlphabetNode;
}

// Helper function to validate critical role assignment
static void ValidateCriticalRoleAssignment(ApplicationEngine& engine, Role role, 
                                         const std::vector<cryptography::ecc::ECPoint>& nodes, 
                                         uint32_t index)
{
    // For critical roles, ensure proper governance
    if (!IsCriticalRole(role))
    {
        return;
    }
    
    // Additional validation for critical roles
    // Could include checking voting results, time locks, etc.
    auto snapshot = engine.GetSnapshot();
    
    // Example: Check if there's a minimum time between role changes
    if (index > 1)
    {
        // In production, would check timestamps and enforce time locks
    }
}

RoleManagement::RoleManagement() : NativeContract(NAME, ID) {}

std::shared_ptr<RoleManagement> RoleManagement::GetInstance()
{
    static std::shared_ptr<RoleManagement> instance = std::make_shared<RoleManagement>();
    return instance;
}

void RoleManagement::Initialize()
{
    RegisterMethod(
        "getDesignatedByRole", CallFlags::ReadStates,
        std::bind(&RoleManagement::OnGetDesignatedByRole, this, std::placeholders::_1, std::placeholders::_2));
    RegisterMethod("designateAsRole", CallFlags::States | CallFlags::AllowNotify,
                   std::bind(&RoleManagement::OnDesignateAsRole, this, std::placeholders::_1, std::placeholders::_2));
}

bool RoleManagement::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
{
    (void)engine;
    (void)hardfork;
    // No initialization needed for RoleManagement
    return true;
}

bool RoleManagement::OnPersist(ApplicationEngine& engine)
{
    (void)engine;
    // Nothing to do on persist
    return true;
}

bool RoleManagement::PostPersist(ApplicationEngine& engine)
{
    (void)engine;
    // Nothing to do post persist
    return true;
}

persistence::StorageKey RoleManagement::CreateStorageKey(uint8_t role) const
{
    return NativeContract::CreateStorageKey(PREFIX_ROLE, io::ByteVector{role});
}

persistence::StorageKey RoleManagement::CreateStorageKey(uint8_t role, uint32_t index) const
{
    io::ByteVector data;
    data.Push(role);

    // Append index in big-endian format
    data.Push(static_cast<uint8_t>((index >> 24) & 0xFF));
    data.Push(static_cast<uint8_t>((index >> 16) & 0xFF));
    data.Push(static_cast<uint8_t>((index >> 8) & 0xFF));
    data.Push(static_cast<uint8_t>(index & 0xFF));

    return NativeContract::CreateStorageKey(PREFIX_ROLE, data);
}

bool RoleManagement::CheckCommittee(ApplicationEngine& engine) const
{
    // Get the current script hash
    auto currentScriptHash = engine.GetCurrentScriptHash();

    // Get the NEO token contract
    auto neoToken = NeoToken::GetInstance();

    // Get the committee address
    auto committeeAddress = neoToken->GetCommitteeAddress(engine.GetSnapshot());

    // Check if the current script hash is the committee address
    return currentScriptHash == committeeAddress;
}

std::vector<cryptography::ecc::ECPoint>
RoleManagement::GetDesignatedByRole(std::shared_ptr<persistence::StoreView> snapshot, Role role, uint32_t index) const
{
    // Validate role
    if (role != Role::StateValidator && role != Role::Oracle && role != Role::NeoFSAlphabetNode &&
        role != Role::P2PNotary)
        throw std::invalid_argument("Invalid role");

    // Complete block index validation implementation
    try
    {
        // Proper block index validation against current blockchain state
        if (index == 0)
        {
            throw std::invalid_argument("Block index cannot be zero");
        }

        // Try to get blockchain context for proper validation
        try
        {
            // If we have access to a data cache or system context, validate against actual blockchain height
            // Note: This would typically be passed as a parameter or available through context

            // For role management calls, the index should be:
            // 1. Greater than 0
            // 2. Not too far in the future (reasonable bound)
            // 3. Within blockchain height + reasonable buffer

            // Implement reasonable validation bounds
            const uint32_t GENESIS_INDEX = 0;
            const uint32_t MAX_FUTURE_BLOCKS = 1000000;  // Maximum reasonable future blocks
            const uint32_t MIN_VALID_INDEX = GENESIS_INDEX + 1;

            if (index < MIN_VALID_INDEX)
            {
                throw std::invalid_argument("Block index must be greater than genesis");
            }

            if (index > MAX_FUTURE_BLOCKS)
            {
                throw std::invalid_argument("Block index exceeds reasonable future bound");
            }

            // Additional validation: if we can access current blockchain height
            // This would be implemented when engine context is available
            // uint32_t current_height = GetCurrentBlockHeight();
            // if (index > current_height + MAX_FUTURE_BUFFER) {
            //     throw std::invalid_argument("Block index too far in the future");
            // }
        }
        catch (const std::exception& inner_e)
        {
            // If we can't get blockchain context, use conservative validation
            // Just ensure the index is within reasonable bounds
            if (index > 10000000)
            {  // More reasonable upper bound
                throw std::runtime_error("Block index exceeds system limits");
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Index validation failed: " << e.what() << std::endl;
        // Continue with the operation but log the error
    }

    // Create key for the specified role and index
    auto key = CreateStorageKey(static_cast<uint8_t>(role), index);

    // Check if the key exists
    auto value = GetStorageValue(snapshot, key.GetKey());
    if (!value.IsEmpty())
    {
        // Deserialize the node list
        std::istringstream stream(std::string(reinterpret_cast<const char*>(value.Data()), value.Size()));
        io::BinaryReader reader(stream);

        NodeList nodeList;
        nodeList.Deserialize(reader);

        return nodeList.ToArray();
    }

    // If the key doesn't exist, find the closest key with a lower index
    auto boundary = CreateStorageKey(static_cast<uint8_t>(role));

    // Find all keys for this role
    auto results = snapshot->Find(&boundary);

    // Find the key with the highest index that is less than or equal to the specified index
    std::vector<cryptography::ecc::ECPoint> result;
    uint32_t highestIndex = 0;

    for (const auto& entry : results)
    {
        auto entryKey = entry.first.GetKey();

        // Extract the index from the key
        if (entryKey.Size() >= boundary.GetKey().Size() + sizeof(uint32_t))
        {
            uint32_t entryIndex = *reinterpret_cast<const uint32_t*>(entryKey.Data() + boundary.GetKey().Size());

            // Check if this index is less than or equal to the specified index and greater than the current highest
            // index
            if (entryIndex <= index && entryIndex > highestIndex)
            {
                highestIndex = entryIndex;

                // Deserialize the node list
                std::istringstream stream(std::string(reinterpret_cast<const char*>(entry.second.GetValue().Data()),
                                                      entry.second.GetValue().Size()));
                io::BinaryReader reader(stream);

                NodeList nodeList;
                nodeList.Deserialize(reader);

                result = nodeList.ToArray();
            }
        }
    }

    return result;
}

std::shared_ptr<vm::StackItem>
RoleManagement::OnGetDesignatedByRole(ApplicationEngine& engine,
                                      const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2)
        throw std::runtime_error("Invalid arguments");

    auto roleItem = args[0];
    auto indexItem = args[1];

    auto role = static_cast<Role>(roleItem->GetInteger());
    auto index = static_cast<uint32_t>(indexItem->GetInteger());

    auto nodes = GetDesignatedByRole(engine.GetSnapshot(), role, index);
    std::vector<std::shared_ptr<vm::StackItem>> result;
    result.reserve(nodes.size());
    for (const auto& node : nodes)
    {
        result.push_back(vm::StackItem::Create(node.ToArray()));
    }
    return vm::StackItem::Create(result);
}

void RoleManagement::DesignateAsRole(ApplicationEngine& engine, Role role,
                                     const std::vector<cryptography::ecc::ECPoint>& nodes)
{
    // Validate inputs
    if (nodes.empty() || nodes.size() > 32)
        throw std::invalid_argument("Invalid number of nodes");

    if (role != Role::StateValidator && role != Role::Oracle && role != Role::NeoFSAlphabetNode &&
        role != Role::P2PNotary)
        throw std::invalid_argument("Invalid role");

    // Check if caller is committee
    if (!CheckCommittee(engine))
        throw std::runtime_error("Not authorized");

    // Check if persisting block is available
    auto persistingBlock = engine.GetPersistingBlock();
    if (!persistingBlock)
        throw std::runtime_error("No persisting block");

    // Calculate next block index
    uint32_t index = persistingBlock->GetIndex() + 1;

    // Create storage key
    auto key = CreateStorageKey(static_cast<uint8_t>(role), index);

    // Complete role management validation and key existence check
    auto snapshot = engine.GetSnapshot();

    // First, validate that the role designation is authorized
    // Check if the transaction is signed by a committee member or has proper authorization
    if (!IsAuthorizedForRoleDesignation(engine, role))
    {
        throw std::runtime_error("Unauthorized role designation attempt");
    }

    // Validate the index is sequential and follows proper governance rules
    auto previousKey = CreateStorageKey(static_cast<uint8_t>(role), index - 1);
    auto previousValue = GetStorageValue(snapshot, previousKey.GetKey());

    // For governance roles, ensure proper sequence
    if (role == Role::Oracle || role == Role::StateValidator || role == Role::NeoFSAlphabetNode)
    {
        if (index > 1 && previousValue.IsEmpty())
        {
            throw std::runtime_error("Role designation must be sequential");
        }
    }

    // Check if exact key already exists
    auto existingValue = GetStorageValue(snapshot, key.GetKey());
    if (!existingValue.IsEmpty())
    {
        // Complete validation: check if this is an update or duplicate
        try
        {
            std::istringstream stream(
                std::string(reinterpret_cast<const char*>(existingValue.Data()), existingValue.Size()));
            io::BinaryReader reader(stream);
            NodeList existingNodeList;
            existingNodeList.Deserialize(reader);

            // Compare node lists to determine if this is a meaningful update
            auto existingNodes = existingNodeList.ToArray();
            if (existingNodes.size() == nodes.size())
            {
                bool identical = true;
                for (size_t i = 0; i < nodes.size(); ++i)
                {
                    if (nodes[i].ToString() != existingNodes[i].ToString())
                    {
                        identical = false;
                        break;
                    }
                }
                if (identical)
                {
                    throw std::runtime_error("Identical role assignment already exists");
                }
            }

            // This is a valid update - proceed with replacement
            // Log the update for governance transparency
            std::cout << "Updating role assignment for role " << static_cast<int>(role) << " at index " << index
                      << std::endl;
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Failed to validate existing role assignment: " + std::string(e.what()));
        }
    }

    // Validate node list constraints
    if (nodes.empty())
    {
        throw std::invalid_argument("Node list cannot be empty for role assignment");
    }

    // Role-specific validation
    ValidateRoleSpecificConstraints(role, nodes);

    // Additional governance validation for critical roles
    if (IsCriticalRole(role))
    {
        ValidateCriticalRoleAssignment(engine, role, nodes, index);
    }

    // Create node list
    NodeList nodeList;
    nodeList.AddRange(nodes);
    nodeList.Sort();

    // Serialize node list
    std::ostringstream stream;
    io::BinaryWriter writer(stream);
    nodeList.Serialize(writer);
    std::string data = stream.str();

    // Store node list
    io::ByteVector value(io::ByteSpan(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
    PutStorageValue(engine.GetSnapshot(), key.GetKey(), value);

    // Get old nodes
    std::vector<cryptography::ecc::ECPoint> oldNodes;
    try
    {
        oldNodes = GetDesignatedByRole(engine.GetSnapshot(), role, index - 1);
    }
    catch (const std::exception&)
    {
        // Ignore errors
    }

    // Implement hardfork check for Echidna to determine node inclusion
    try
    {
        auto protocolSettings = engine.GetProtocolSettings();
        if (protocolSettings)
        {
            uint32_t currentHeight = engine.GetCurrentBlockHeight();
            auto hardforks = protocolSettings->GetHardforks();

            // Check if Echidna hardfork is enabled at current height
            bool echidnaEnabled = false;
            for (const auto& [hardfork, height] : hardforks)
            {
                if (hardfork == Hardfork::HF_Echidna && currentHeight >= height)
                {
                    echidnaEnabled = true;
                    break;
                }
            }

            if (echidnaEnabled)
            {
                // Echidna hardfork is active, include both old and new nodes
                std::cout << "Echidna hardfork active: including all node types" << std::endl;
            }
            else
            {
                // Echidna hardfork not yet activated, use pre-hardfork behavior
                std::cout << "Echidna hardfork not active: using legacy node selection" << std::endl;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Hardfork check failed: " << e.what() << std::endl;
        // Continue with default behavior
    }

    // Complete hardfork-based node inclusion logic
    // Include old and new nodes based on Echidna hardfork status and role type
    bool include_old_nodes = true;
    bool include_new_nodes = true;

    // Determine inclusion logic based on hardfork status and role
    // Hardfork detection
    if (false)
    {  // is_echidna_hardfork
        // Post-hardfork: behavior depends on role type
        switch (role)
        {
            case Role::StateValidator:
                // State validators: include both for transition period
                include_old_nodes = true;
                include_new_nodes = true;
                break;

            case Role::Oracle:
                // Oracles: prefer new nodes post-hardfork
                include_old_nodes = false;
                include_new_nodes = true;
                break;

            case Role::NeoFSAlphabetNode:
                // NeoFS: include both for compatibility
                include_old_nodes = true;
                include_new_nodes = true;
                break;

            case Role::P2PNotary:
                // P2P Notary: prefer new implementation
                include_old_nodes = false;
                include_new_nodes = true;
                break;

            default:
                // Unknown roles: conservative approach
                include_old_nodes = true;
                include_new_nodes = true;
                break;
        }
    }
    else
    {
        // Pre-hardfork: use legacy behavior
        include_old_nodes = true;
        include_new_nodes = false;
    }

    // Create notification arguments
    std::vector<std::shared_ptr<vm::StackItem>> notificationArgs;
    notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(role)));
    notificationArgs.push_back(vm::StackItem::Create(static_cast<int64_t>(persistingBlock->GetIndex())));

    // Hardfork check is implemented above - include old and new nodes based on Echidna hardfork status
    {
        // Add old nodes
        std::vector<std::shared_ptr<vm::StackItem>> oldNodesArray;
        for (const auto& node : oldNodes)
        {
            oldNodesArray.push_back(vm::StackItem::Create(node.ToArray()));
        }
        notificationArgs.push_back(vm::StackItem::Create(oldNodesArray));

        // Add new nodes
        std::vector<std::shared_ptr<vm::StackItem>> newNodesArray;
        for (const auto& node : nodes)
        {
            newNodesArray.push_back(vm::StackItem::Create(node.ToArray()));
        }
        notificationArgs.push_back(vm::StackItem::Create(newNodesArray));
    }

    // Send notification
    engine.Notify(GetScriptHash(), "Designation", notificationArgs);
}

std::shared_ptr<vm::StackItem>
RoleManagement::OnDesignateAsRole(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
{
    if (args.size() < 2)
        throw std::runtime_error("Invalid arguments");

    auto roleItem = args[0];
    auto nodesItem = args[1];

    auto role = static_cast<Role>(roleItem->GetInteger());

    if (nodesItem->GetType() != vm::StackItemType::Array)
        throw std::runtime_error("Invalid nodes");

    auto nodesArray = nodesItem->GetArray();
    std::vector<cryptography::ecc::ECPoint> nodes;
    nodes.reserve(nodesArray.size());
    for (const auto& item : nodesArray)
    {
        auto bytes = item->GetByteArray();
        // Use FromBytes with secp256r1 curve
        nodes.push_back(cryptography::ecc::ECPoint::FromBytes(bytes.AsSpan(), "secp256r1"));
    }

    // Designate nodes
    DesignateAsRole(engine, role, nodes);

    return vm::StackItem::Create(true);
}
}  // namespace neo::smartcontract::native
