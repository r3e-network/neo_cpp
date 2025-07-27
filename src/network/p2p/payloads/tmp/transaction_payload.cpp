#include <neo/network/payloads/transaction_payload.h>

namespace neo::network::payloads
{
TransactionPayload::TransactionPayload() = default;

TransactionPayload::TransactionPayload(std::shared_ptr<blockchain::Transaction> transaction) : transaction_(transaction)
{
}

std::shared_ptr<blockchain::Transaction> TransactionPayload::GetTransaction() const
{
    return transaction_;
}

void TransactionPayload::SetTransaction(std::shared_ptr<blockchain::Transaction> transaction)
{
    transaction_ = transaction;
}

void TransactionPayload::Serialize(io::BinaryWriter& writer) const
{
    if (transaction_)
    {
        writer.Write(*transaction_);
    }
}

void TransactionPayload::Deserialize(io::BinaryReader& reader)
{
    transaction_ = std::make_shared<blockchain::Transaction>();
    transaction_->Deserialize(reader);
}

void TransactionPayload::SerializeJson(io::JsonWriter& writer) const
{
    if (transaction_)
    {
        writer.WriteStartObject("transaction");
        transaction_->SerializeJson(writer);
        writer.WriteEndObject();
    }
    else
    {
        writer.Write("transaction", nullptr);
    }
}

void TransactionPayload::DeserializeJson(const io::JsonReader& reader)
{
    if (reader.HasField("transaction") && !reader.IsNull("transaction"))
    {
        transaction_ = std::make_shared<blockchain::Transaction>();
        io::JsonReader transactionReader = reader.ReadObject("transaction");
        transaction_->DeserializeJson(transactionReader);
    }
}
}  // namespace neo::network::payloads
