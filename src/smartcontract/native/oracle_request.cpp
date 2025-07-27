#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/smartcontract/native/oracle_request.h>
#include <sstream>

namespace neo::smartcontract::native
{
OracleRequest::OracleRequest() : gasForResponse_(0) {}

OracleRequest::OracleRequest(const io::UInt256& originalTxid, int64_t gasForResponse, const std::string& url,
                             const std::string& filter, const io::UInt160& callbackContract,
                             const std::string& callbackMethod, const io::ByteVector& userData)
    : originalTxid_(originalTxid), gasForResponse_(gasForResponse), url_(url), filter_(filter),
      callbackContract_(callbackContract), callbackMethod_(callbackMethod), userData_(userData)
{
}

const io::UInt256& OracleRequest::GetOriginalTxid() const
{
    return originalTxid_;
}

void OracleRequest::SetOriginalTxid(const io::UInt256& originalTxid)
{
    originalTxid_ = originalTxid;
}

int64_t OracleRequest::GetGasForResponse() const
{
    return gasForResponse_;
}

void OracleRequest::SetGasForResponse(int64_t gasForResponse)
{
    gasForResponse_ = gasForResponse;
}

const std::string& OracleRequest::GetUrl() const
{
    return url_;
}

void OracleRequest::SetUrl(const std::string& url)
{
    url_ = url;
}

const std::string& OracleRequest::GetFilter() const
{
    return filter_;
}

void OracleRequest::SetFilter(const std::string& filter)
{
    filter_ = filter;
}

const io::UInt160& OracleRequest::GetCallbackContract() const
{
    return callbackContract_;
}

void OracleRequest::SetCallbackContract(const io::UInt160& callbackContract)
{
    callbackContract_ = callbackContract;
}

const std::string& OracleRequest::GetCallbackMethod() const
{
    return callbackMethod_;
}

void OracleRequest::SetCallbackMethod(const std::string& callbackMethod)
{
    callbackMethod_ = callbackMethod;
}

const io::ByteVector& OracleRequest::GetUserData() const
{
    return userData_;
}

void OracleRequest::SetUserData(const io::ByteVector& userData)
{
    userData_ = userData;
}

std::shared_ptr<vm::StackItem> OracleRequest::ToStackItem() const
{
    std::vector<std::shared_ptr<vm::StackItem>> items;
    items.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(originalTxid_.Data(), io::UInt256::Size))));
    items.push_back(vm::StackItem::Create(gasForResponse_));
    items.push_back(vm::StackItem::Create(url_));
    items.push_back(filter_.empty() ? vm::StackItem::Null() : vm::StackItem::Create(filter_));
    items.push_back(vm::StackItem::Create(io::ByteVector(io::ByteSpan(callbackContract_.Data(), io::UInt160::Size))));
    items.push_back(vm::StackItem::Create(callbackMethod_));
    items.push_back(vm::StackItem::Create(userData_));
    return vm::StackItem::Create(items);
}

void OracleRequest::FromStackItem(const std::shared_ptr<vm::StackItem>& item)
{
    if (!item->IsArray())
        throw std::runtime_error("Expected array");

    auto array = item->GetArray();
    if (array.size() < 7)
        throw std::runtime_error("Invalid array size");

    auto originalTxidBytes = array[0]->GetByteArray();
    if (originalTxidBytes.Size() != io::UInt256::Size)
        throw std::runtime_error("Invalid originalTxid size");
    std::memcpy(originalTxid_.Data(), originalTxidBytes.Data(), io::UInt256::Size);

    gasForResponse_ = array[1]->GetInteger();
    url_ = array[2]->GetString();
    filter_ = array[3]->IsNull() ? "" : array[3]->GetString();

    auto callbackContractBytes = array[4]->GetByteArray();
    if (callbackContractBytes.Size() != io::UInt160::Size)
        throw std::runtime_error("Invalid callbackContract size");
    std::memcpy(callbackContract_.Data(), callbackContractBytes.Data(), io::UInt160::Size);

    callbackMethod_ = array[5]->GetString();
    userData_ = array[6]->GetByteArray();
}

void OracleRequest::Serialize(io::BinaryWriter& writer) const
{
    writer.Write(originalTxid_);
    writer.Write(gasForResponse_);
    writer.WriteVarString(url_);
    writer.WriteVarString(filter_);
    writer.Write(callbackContract_);
    writer.WriteVarString(callbackMethod_);
    writer.WriteVarBytes(userData_.AsSpan());
}

void OracleRequest::Deserialize(io::BinaryReader& reader)
{
    originalTxid_ = reader.ReadSerializable<io::UInt256>();
    gasForResponse_ = reader.ReadInt64();
    url_ = reader.ReadVarString();
    filter_ = reader.ReadVarString();
    callbackContract_ = reader.ReadSerializable<io::UInt160>();
    callbackMethod_ = reader.ReadVarString();
    userData_ = reader.ReadVarBytes();
}
}  // namespace neo::smartcontract::native
