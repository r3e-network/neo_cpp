#include <neo/network/p2p/payloads/transaction_payload.h>

namespace neo::network::p2p::payloads
{
    TransactionPayload::TransactionPayload() = default;

    TransactionPayload::TransactionPayload(std::shared_ptr<ledger::Transaction> transaction)
        : transaction_(transaction)
    {
    }

    std::shared_ptr<ledger::Transaction> TransactionPayload::GetTransaction() const
    {
        return transaction_;
    }

    void TransactionPayload::SetTransaction(std::shared_ptr<ledger::Transaction> transaction)
    {
        transaction_ = transaction;
    }

    int TransactionPayload::GetSize() const
    {
        return transaction_ ? transaction_->GetSize() : 0;
    }

    TransactionPayload TransactionPayload::Create(std::shared_ptr<ledger::Transaction> transaction)
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
        transaction_ = std::make_shared<ledger::Transaction>();
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
            transaction_ = std::make_shared<ledger::Transaction>();
            io::JsonReader transactionReader(reader.GetJson()["transaction"]);
            transaction_->DeserializeJson(transactionReader);
        }
    }
}
