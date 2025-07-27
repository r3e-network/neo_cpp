#include <neo/wallets/wallet_transaction.h>

namespace neo::wallets
{
WalletTransaction::WalletTransaction(std::shared_ptr<Neo3Transaction> transaction)
    : transaction_(transaction), confirmed_(false), block_height_(0)
{
}

io::UInt256 WalletTransaction::GetHash() const
{
    if (transaction_)
    {
        return transaction_->GetHash();
    }
    return io::UInt256::Zero();
}

void WalletTransaction::SerializeJson(io::JsonWriter& writer) const
{
    // Complete JSON serialization implementation for WalletTransaction
    try
    {
        writer.WriteStartObject();

        // Write transaction hash
        if (transaction_)
        {
            writer.WritePropertyName("txid");
            writer.WriteValue(transaction_->GetHash().ToString());

            // Write basic transaction info
            writer.WritePropertyName("size");
            writer.WriteValue(static_cast<int32_t>(transaction_->GetSize()));

            writer.WritePropertyName("version");
            writer.WriteValue(static_cast<int32_t>(transaction_->GetVersion()));

            writer.WritePropertyName("nonce");
            writer.WriteValue(transaction_->GetNonce());

            writer.WritePropertyName("sender");
            if (!transaction_->GetSigners().empty())
            {
                writer.WriteValue(transaction_->GetSigners()[0].GetAccount().ToString());
            }
            else
            {
                writer.WriteNull();
            }

            writer.WritePropertyName("sysfee");
            writer.WriteValue(std::to_string(transaction_->GetSystemFee()));

            writer.WritePropertyName("netfee");
            writer.WriteValue(std::to_string(transaction_->GetNetworkFee()));

            writer.WritePropertyName("validuntilblock");
            writer.WriteValue(transaction_->GetValidUntilBlock());
        }

        // Write wallet-specific fields
        writer.WritePropertyName("confirmed");
        writer.WriteValue(confirmed_);

        if (confirmed_)
        {
            writer.WritePropertyName("blockheight");
            writer.WriteValue(block_height_);

            writer.WritePropertyName("blockhash");
            writer.WriteValue(block_hash_.ToString());

            writer.WritePropertyName("blocktime");
            writer.WriteValue(block_time_);

            writer.WritePropertyName("vmstate");
            std::string vm_state_str;
            switch (vm_state_)
            {
                case vm::VMState::Halt:
                    vm_state_str = "HALT";
                    break;
                case vm::VMState::Fault:
                    vm_state_str = "FAULT";
                    break;
                case vm::VMState::Break:
                    vm_state_str = "BREAK";
                    break;
                default:
                    vm_state_str = "NONE";
                    break;
            }
            writer.WriteValue(vm_state_str);

            if (!exception_.empty())
            {
                writer.WritePropertyName("exception");
                writer.WriteValue(exception_);
            }

            writer.WritePropertyName("gasconsumed");
            writer.WriteValue(std::to_string(gas_consumed_));
        }

        writer.WriteEndObject();
    }
    catch (const std::exception& e)
    {
        // If JSON serialization fails, write minimal object
        writer.WriteStartObject();
        writer.WritePropertyName("error");
        writer.WriteValue("Serialization failed: " + std::string(e.what()));
        writer.WriteEndObject();
    }
}

void WalletTransaction::DeserializeJson(const io::JsonReader& reader)
{
    // Complete JSON deserialization implementation for WalletTransaction
    try
    {
        // Start reading the JSON object
        reader.ReadStartObject();

        while (reader.Read())
        {
            if (reader.TokenType() == io::JsonToken::EndObject)
            {
                break;
            }

            if (reader.TokenType() == io::JsonToken::PropertyName)
            {
                std::string propertyName = reader.GetString();
                reader.Read();  // Move to property value

                if (propertyName == "confirmed")
                {
                    confirmed_ = reader.GetBoolean();
                }
                else if (propertyName == "blockheight" || propertyName == "block_height")
                {
                    block_height_ = reader.GetUInt32();
                }
                else if (propertyName == "blockhash" || propertyName == "block_hash")
                {
                    if (reader.TokenType() != io::JsonToken::Null)
                    {
                        std::string blockHashStr = reader.GetString();
                        block_hash_ = io::UInt256::Parse(blockHashStr);
                    }
                }
                else if (propertyName == "blocktime" || propertyName == "block_time")
                {
                    block_time_ = reader.GetUInt64();
                }
                else if (propertyName == "vmstate" || propertyName == "vm_state")
                {
                    std::string vmStateStr = reader.GetString();
                    if (vmStateStr == "HALT")
                    {
                        vm_state_ = vm::VMState::Halt;
                    }
                    else if (vmStateStr == "FAULT")
                    {
                        vm_state_ = vm::VMState::Fault;
                    }
                    else if (vmStateStr == "BREAK")
                    {
                        vm_state_ = vm::VMState::Break;
                    }
                    else
                    {
                        vm_state_ = vm::VMState::None;
                    }
                }
                else if (propertyName == "exception")
                {
                    if (reader.TokenType() != io::JsonToken::Null)
                    {
                        exception_ = reader.GetString();
                    }
                    else
                    {
                        exception_.clear();
                    }
                }
                else if (propertyName == "gasconsumed" || propertyName == "gas_consumed")
                {
                    if (reader.TokenType() == io::JsonToken::String)
                    {
                        gas_consumed_ = std::stoll(reader.GetString());
                    }
                    else
                    {
                        gas_consumed_ = reader.GetInt64();
                    }
                }
                else
                {
                    // Skip unknown properties
                    reader.Skip();
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        // Error parsing JSON - set safe default values
        confirmed_ = false;
        block_height_ = 0;
        block_hash_ = io::UInt256::Zero();
        block_time_ = 0;
        vm_state_ = vm::VMState::None;
        exception_.clear();
        gas_consumed_ = 0;

        throw std::runtime_error("Failed to deserialize WalletTransaction from JSON: " + std::string(e.what()));
    }
}

}  // namespace neo::wallets