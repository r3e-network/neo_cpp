#include <neo/network/payloads/filter_load_payload.h>
#include <stdexcept>

namespace neo::network::payloads
{
    FilterLoadPayload::FilterLoadPayload()
        : k_(0), tweak_(0), flags_(0)
    {
    }

    FilterLoadPayload::FilterLoadPayload(const io::ByteVector& filter, uint8_t k, uint32_t tweak, uint8_t flags)
        : filter_(filter), k_(k), tweak_(tweak), flags_(flags)
    {
        if (filter.Size() > MaxFilterSize)
            throw std::out_of_range("Filter exceeds maximum size");
    }

    const io::ByteVector& FilterLoadPayload::GetFilter() const
    {
        return filter_;
    }

    void FilterLoadPayload::SetFilter(const io::ByteVector& filter)
    {
        if (filter.Size() > MaxFilterSize)
            throw std::out_of_range("Filter exceeds maximum size");
        
        filter_ = filter;
    }

    uint8_t FilterLoadPayload::GetK() const
    {
        return k_;
    }

    void FilterLoadPayload::SetK(uint8_t k)
    {
        k_ = k;
    }

    uint32_t FilterLoadPayload::GetTweak() const
    {
        return tweak_;
    }

    void FilterLoadPayload::SetTweak(uint32_t tweak)
    {
        tweak_ = tweak;
    }

    uint8_t FilterLoadPayload::GetFlags() const
    {
        return flags_;
    }

    void FilterLoadPayload::SetFlags(uint8_t flags)
    {
        flags_ = flags;
    }

    void FilterLoadPayload::Serialize(io::BinaryWriter& writer) const
    {
        writer.WriteVarBytes(filter_.AsSpan());
        writer.Write(k_);
        writer.Write(tweak_);
        writer.Write(flags_);
    }

    void FilterLoadPayload::Deserialize(io::BinaryReader& reader)
    {
        filter_ = reader.ReadVarBytes(MaxFilterSize);
        k_ = reader.ReadUInt8();
        tweak_ = reader.ReadUInt32();
        flags_ = reader.ReadUInt8();
    }

    void FilterLoadPayload::SerializeJson(io::JsonWriter& writer) const
    {
        writer.Write("filter", filter_);
        writer.Write("k", k_);
        writer.Write("tweak", tweak_);
        writer.Write("flags", flags_);
    }

    void FilterLoadPayload::DeserializeJson(const io::JsonReader& reader)
    {
        filter_ = reader.ReadByteVector("filter");
        k_ = reader.ReadUInt8("k");
        tweak_ = reader.ReadUInt32("tweak");
        flags_ = reader.ReadUInt8("flags");
    }
}
