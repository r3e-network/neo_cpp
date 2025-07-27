#include <neo/network/p2p/payloads/transaction_payload.h>

namespace neo::network::p2p::payloads
{
TransactionPayload::TransactionPayload() = default;

TransactionPayload::TransactionPayload(std::shared_ptr<Neo3Transaction> transaction) : transaction_(transaction) {}

std::shared_ptr<Neo3Transaction> TransactionPayload::GetTransaction() const
{
    return transaction_;
}

void TransactionPayload::SetTransaction(std::shared_ptr<Neo3Transaction> transaction)
{
    transaction_ = transaction;
}

int TransactionPayload::GetSize() const
{
    return transaction_ ? static_cast<int>(transaction_->GetSize()) : 0;
}

TransactionPayload TransactionPayload::Create(std::shared_ptr<Neo3Transaction> transaction)
{
    return TransactionPayload(transaction);
}

void TransactionPayload::Serialize(io::BinaryWriter& writer) const
{
    if (transaction_)
    {
        transaction_->Serialize(writer);
    }
}

void TransactionPayload::Deserialize(io::BinaryReader& reader)
{
    transaction_ = std::make_shared<Neo3Transaction>();
    transaction_->Deserialize(reader);
}

void TransactionPayload::SerializeJson(io::JsonWriter& writer) const
{
    writer.WriteStartObject();
    if (transaction_)
    {
        nlohmann::json transactionJson = nlohmann::json::object();
        io::JsonWriter transactionWriter(transactionJson);
        transaction_->SerializeJson(transactionWriter);
        writer.WriteProperty("transaction", transactionJson);
    }
    writer.WriteEndObject();
}

void TransactionPayload::DeserializeJson(const io::JsonReader& reader)
{
    if (reader.GetJson().contains("transaction") && reader.GetJson()["transaction"].is_object())
    {
        transaction_ = std::make_shared<Neo3Transaction>();
        io::JsonReader transactionReader(reader.GetJson()["transaction"]);
        transaction_->DeserializeJson(transactionReader);
    }
}
}  // namespace neo::network::p2p::payloads
