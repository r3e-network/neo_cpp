#include <neo/wallets/wallet_transaction.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/network/p2p/payloads/transaction_factory.h>

namespace neo::wallets
{
    WalletTransaction::WalletTransaction()
        : height_(0), time_(std::chrono::system_clock::now())
    {
    }

    WalletTransaction::WalletTransaction(const network::p2p::payloads::Transaction& transaction)
        : hash_(transaction.GetHash()), height_(0), time_(std::chrono::system_clock::now())
    {
        transaction_ = std::make_shared<network::p2p::payloads::Transaction>(transaction);
    }

    WalletTransaction::WalletTransaction(const network::p2p::payloads::Transaction& transaction, uint32_t height)
        : hash_(transaction.GetHash()), height_(height), time_(std::chrono::system_clock::now())
    {
        transaction_ = std::make_shared<network::p2p::payloads::Transaction>(transaction);
    }

    const io::UInt256& WalletTransaction::GetHash() const
    {
        return hash_;
    }

    void WalletTransaction::SetHash(const io::UInt256& hash)
    {
        hash_ = hash;
    }

    const std::shared_ptr<network::p2p::payloads::Transaction>& WalletTransaction::GetTransaction() const
    {
        return transaction_;
    }

    void WalletTransaction::SetTransaction(const std::shared_ptr<network::p2p::payloads::Transaction>& transaction)
    {
        transaction_ = transaction;
        if (transaction)
        {
            hash_ = transaction->GetHash();
        }
    }

    uint32_t WalletTransaction::GetHeight() const
    {
        return height_;
    }

    void WalletTransaction::SetHeight(uint32_t height)
    {
        height_ = height;
    }

    const std::chrono::system_clock::time_point& WalletTransaction::GetTime() const
    {
        return time_;
    }

    void WalletTransaction::SetTime(const std::chrono::system_clock::time_point& time)
    {
        time_ = time;
    }

    void WalletTransaction::SerializeJson(io::JsonWriter& writer) const
    {
        writer.WriteStartObject();

        writer.WritePropertyName("hash");
        writer.WriteString(hash_.ToString());

        if (transaction_)
        {
            writer.WritePropertyName("transaction");
            // Serialize the transaction to a byte array and then to base64
            std::stringstream stream;
            io::BinaryWriter binaryWriter(stream);
            transaction_->Serialize(binaryWriter);
            writer.WriteBase64String(reinterpret_cast<const uint8_t*>(stream.str().data()), stream.str().size());
        }

        writer.WritePropertyName("height");
        writer.WriteNumber(height_);

        writer.WritePropertyName("time");
        // Convert time_point to milliseconds since epoch
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_.time_since_epoch()).count();
        writer.WriteNumber(milliseconds);

        writer.WriteEndObject();
    }

    void WalletTransaction::DeserializeJson(const io::JsonReader& reader)
    {
        hash_ = io::UInt256::Parse(reader.ReadString("hash"));

        if (reader.HasProperty("transaction"))
        {
            // Deserialize the transaction from base64
            auto transactionBase64 = reader.ReadBase64String("transaction");
            std::stringstream stream(std::string(reinterpret_cast<const char*>(transactionBase64.Data()), transactionBase64.Size()));
            io::BinaryReader binaryReader(stream);
            transaction_ = network::p2p::payloads::TransactionFactory::DeserializeFrom(binaryReader);
        }

        height_ = static_cast<uint32_t>(reader.ReadNumber("height"));

        // Convert milliseconds since epoch to time_point
        auto milliseconds = static_cast<int64_t>(reader.ReadNumber("time"));
        time_ = std::chrono::system_clock::time_point(std::chrono::milliseconds(milliseconds));
    }
}
