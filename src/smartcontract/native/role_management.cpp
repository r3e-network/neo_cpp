#include <neo/smartcontract/native/role_management.h>
#include <neo/smartcontract/application_engine.h>
#include <neo/smartcontract/native/neo_token.h>
#include <neo/persistence/storage_key.h>
#include <neo/persistence/storage_item.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/cryptography/hash.h>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace neo::smartcontract::native
{
    RoleManagement::RoleManagement()
        : NativeContract(NAME, ID)
    {
    }

    std::shared_ptr<RoleManagement> RoleManagement::GetInstance()
    {
        static std::shared_ptr<RoleManagement> instance = std::make_shared<RoleManagement>();
        return instance;
    }

    void RoleManagement::Initialize()
    {
        RegisterMethod("getDesignatedByRole", CallFlags::ReadStates, std::bind(&RoleManagement::OnGetDesignatedByRole, this, std::placeholders::_1, std::placeholders::_2));
        RegisterMethod("designateAsRole", CallFlags::States | CallFlags::AllowNotify, std::bind(&RoleManagement::OnDesignateAsRole, this, std::placeholders::_1, std::placeholders::_2));
    }

    bool RoleManagement::InitializeContract(ApplicationEngine& engine, uint32_t hardfork)
    {
        // No initialization needed for RoleManagement
        return true;
    }

    bool RoleManagement::OnPersist(ApplicationEngine& engine)
    {
        // Nothing to do on persist
        return true;
    }

    bool RoleManagement::PostPersist(ApplicationEngine& engine)
    {
        // Nothing to do post persist
        return true;
    }

    persistence::StorageKey RoleManagement::CreateStorageKey(uint8_t role) const
    {
        return GetStorageKey(PREFIX_ROLE, io::ByteVector{role});
    }

    persistence::StorageKey RoleManagement::CreateStorageKey(uint8_t role, uint32_t index) const
    {
        io::ByteVector data;
        data.push_back(role);

        // Append index in big-endian format
        data.push_back(static_cast<uint8_t>((index >> 24) & 0xFF));
        data.push_back(static_cast<uint8_t>((index >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((index >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(index & 0xFF));

        return GetStorageKey(PREFIX_ROLE, data);
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

    std::vector<cryptography::ecc::ECPoint> RoleManagement::GetDesignatedByRole(std::shared_ptr<persistence::StoreView> snapshot, Role role, uint32_t index) const
    {
        // Validate role
        if (role != Role::StateValidator && role != Role::Oracle && role != Role::NeoFSAlphabetNode && role != Role::P2PNotary)
            throw std::invalid_argument("Invalid role");

        // Validate index
        uint32_t currentIndex = snapshot->GetCurrentBlockIndex();
        if (currentIndex + 1 < index)
            throw std::invalid_argument("Invalid index");

        // Create key for the specified role and index
        auto key = CreateStorageKey(static_cast<uint8_t>(role), index);

        // Check if the key exists
        auto value = GetStorageValue(snapshot, key);
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
        auto iterator = snapshot->Find(boundary);

        // Find the key with the highest index that is less than or equal to the specified index
        std::vector<cryptography::ecc::ECPoint> result;
        uint32_t highestIndex = 0;

        while (iterator->HasNext())
        {
            auto entry = iterator->Next();
            auto entryKey = entry.first.GetKey();

            // Extract the index from the key
            if (entryKey.Size() >= boundary.Size() + sizeof(uint32_t))
            {
                uint32_t entryIndex = *reinterpret_cast<const uint32_t*>(entryKey.Data() + boundary.Size());

                // Check if this index is less than or equal to the specified index and greater than the current highest index
                if (entryIndex <= index && entryIndex > highestIndex)
                {
                    highestIndex = entryIndex;

                    // Deserialize the node list
                    std::istringstream stream(std::string(reinterpret_cast<const char*>(entry.second.GetValue().Data()), entry.second.GetValue().Size()));
                    io::BinaryReader reader(stream);

                    NodeList nodeList;
                    nodeList.Deserialize(reader);

                    result = nodeList.ToArray();
                }
            }
        }

        return result;
    }

    std::shared_ptr<vm::StackItem> RoleManagement::OnGetDesignatedByRole(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
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

    void RoleManagement::DesignateAsRole(ApplicationEngine& engine, Role role, const std::vector<cryptography::ecc::ECPoint>& nodes)
    {
        // Validate inputs
        if (nodes.empty() || nodes.size() > 32)
            throw std::invalid_argument("Invalid number of nodes");

        if (role != Role::StateValidator && role != Role::Oracle && role != Role::NeoFSAlphabetNode && role != Role::P2PNotary)
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

        // Check if key already exists
        if (engine.GetSnapshot()->Contains(key))
            throw std::runtime_error("Key already exists");

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
        PutStorageValue(engine.GetSnapshot(), key, value);

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

        // Create notification arguments
        auto notificationArgs = vm::StackItem::CreateArray();
        notificationArgs->Add(vm::StackItem::Create(static_cast<int64_t>(role)));
        notificationArgs->Add(vm::StackItem::Create(static_cast<int64_t>(persistingBlock->GetIndex())));

        // Check if Echidna hardfork is enabled
        if (engine.IsHardforkEnabled(Hardfork::Echidna))
        {
            // Add old nodes
            auto oldNodesArray = vm::StackItem::CreateArray();
            for (const auto& node : oldNodes)
            {
                oldNodesArray->Add(vm::StackItem::Create(node.ToArray()));
            }
            notificationArgs->Add(oldNodesArray);

            // Add new nodes
            auto newNodesArray = vm::StackItem::CreateArray();
            for (const auto& node : nodes)
            {
                newNodesArray->Add(vm::StackItem::Create(node.ToArray()));
            }
            notificationArgs->Add(newNodesArray);
        }

        // Send notification
        engine.SendNotification(GetScriptHash(), "Designation", notificationArgs);
    }

    std::shared_ptr<vm::StackItem> RoleManagement::OnDesignateAsRole(ApplicationEngine& engine, const std::vector<std::shared_ptr<vm::StackItem>>& args)
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
            nodes.push_back(cryptography::ecc::ECPoint::FromBytes(bytes.AsSpan(), cryptography::ecc::ECCurve::Secp256r1));
        }

        // Designate nodes
        DesignateAsRole(engine, role, nodes);

        return vm::StackItem::Create(true);
    }
}
