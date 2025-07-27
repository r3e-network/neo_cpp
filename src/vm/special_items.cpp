#include <algorithm>
#include <neo/vm/exceptions.h>
#include <neo/vm/special_items.h>
#include <stdexcept>

namespace neo::vm
{
// InteropInterfaceItem implementation
InteropInterfaceItem::InteropInterfaceItem(std::shared_ptr<void> value) : value_(value) {}

StackItemType InteropInterfaceItem::GetType() const
{
    return StackItemType::InteropInterface;
}

bool InteropInterfaceItem::GetBoolean() const
{
    return value_ != nullptr;
}

std::shared_ptr<void> InteropInterfaceItem::GetInterface() const
{
    return value_;
}

size_t InteropInterfaceItem::Size() const
{
    return sizeof(void*);
}

size_t InteropInterfaceItem::GetHashCode() const
{
    return std::hash<void*>()(value_.get());
}

bool InteropInterfaceItem::Equals(const StackItem& other) const
{
    if (other.GetType() != StackItemType::InteropInterface)
        return false;

    auto otherInterface = other.GetInterface();
    return value_.get() == otherInterface.get();
}

// PointerItem implementation
PointerItem::PointerItem(int32_t position, std::shared_ptr<StackItem> value) : position_(position), value_(value) {}

StackItemType PointerItem::GetType() const
{
    return StackItemType::Pointer;
}

bool PointerItem::GetBoolean() const
{
    return true;
}

int32_t PointerItem::GetPosition() const
{
    return position_;
}

std::shared_ptr<StackItem> PointerItem::GetValue() const
{
    return value_;
}

bool PointerItem::Equals(const StackItem& other) const
{
    if (other.GetType() != StackItemType::Pointer)
        return false;

    auto otherPointer = dynamic_cast<const PointerItem&>(other);
    return position_ == otherPointer.GetPosition() &&
           ((value_ == nullptr && otherPointer.GetValue() == nullptr) ||
            (value_ != nullptr && otherPointer.GetValue() != nullptr && value_->Equals(*otherPointer.GetValue())));
}

// NullItem implementation
StackItemType NullItem::GetType() const
{
    return StackItemType::Null;
}

bool NullItem::GetBoolean() const
{
    return false;
}

int64_t NullItem::GetInteger() const
{
    throw InvalidOperationException("Cannot convert null to integer");
}

std::shared_ptr<StackItem> NullItem::ConvertTo(StackItemType type) const
{
    if (type == StackItemType::Any || !IsValidStackItemType(type))
        throw InvalidCastException("Type Null can't be converted to StackItemType: " +
                                   std::to_string(static_cast<int>(type)));
    return std::const_pointer_cast<StackItem>(shared_from_this());
}

bool NullItem::Equals(const StackItem& other) const
{
    return other.GetType() == StackItemType::Any;
}
}  // namespace neo::vm
