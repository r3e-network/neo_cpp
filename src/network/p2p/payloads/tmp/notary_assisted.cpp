#include <neo/network/payloads/notary_assisted.h>
#include <neo/network/payloads/transaction.h>
#include <neo/smartcontract/helper.h>

namespace neo::network::payloads
{
NotaryAssisted::NotaryAssisted() : nKeys(0) {}

TransactionAttributeType NotaryAssisted::GetType() const
{
    return TransactionAttributeType::NotaryAssisted;
}

int NotaryAssisted::GetSize() const
{
    return sizeof(uint8_t);
}

uint8_t NotaryAssisted::GetNKeys() const
{
    return nKeys;
}

void NotaryAssisted::SetNKeys(uint8_t value)
{
    nKeys = value;
}

void NotaryAssisted::Deserialize(io::BinaryReader& reader)
{
    nKeys = reader.ReadByte();
}

void NotaryAssisted::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteByte(nKeys);
}

bool NotaryAssisted::Verify(std::shared_ptr<persistence::StoreView> snapshot, const Transaction& tx) const
{
    // Get the notary contract hash
    static const io::UInt160 notaryHash = smartcontract::GetContractHash(io::UInt160(), 0, "Notary");

    // Check if the transaction has the notary contract as a signer
    if (tx.GetSigners().size() < 1 || tx.GetSigners()[0].GetAccount() != notaryHash)
        return false;

    return true;
}
}  // namespace neo::network::payloads
