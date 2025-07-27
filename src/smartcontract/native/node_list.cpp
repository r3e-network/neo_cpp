#include <algorithm>
#include <neo/smartcontract/native/role_management.h>

namespace neo::smartcontract::native
{
void NodeList::Add(const cryptography::ecc::ECPoint& node)
{
    nodes_.push_back(node);
}

void NodeList::AddRange(const std::vector<cryptography::ecc::ECPoint>& nodes)
{
    nodes_.insert(nodes_.end(), nodes.begin(), nodes.end());
}

void NodeList::Sort()
{
    std::sort(nodes_.begin(), nodes_.end());
}

std::vector<cryptography::ecc::ECPoint> NodeList::ToArray() const
{
    return nodes_;
}

void NodeList::Serialize(io::BinaryWriter& writer) const
{
    writer.WriteVarInt(nodes_.size());
    for (const auto& node : nodes_)
    {
        writer.WriteVarBytes(node.ToArray().AsSpan());
    }
}

void NodeList::Deserialize(io::BinaryReader& reader)
{
    uint64_t count = reader.ReadVarInt();
    nodes_.clear();
    nodes_.reserve(count);

    for (uint64_t i = 0; i < count; i++)
    {
        auto bytes = reader.ReadVarBytes(cryptography::ecc::ECPoint::MaxSize);
        nodes_.push_back(cryptography::ecc::ECPoint::FromBytes(bytes.AsSpan(), "secp256r1"));
    }
}
}  // namespace neo::smartcontract::native
