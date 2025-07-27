#pragma once

#include <cstdint>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/vm/stack_item.h>
#include <vector>

namespace neo::smartcontract::native
{
/**
 * @brief Represents a list of IDs.
 */
class IdList
{
  public:
    /**
     * @brief Constructs an IdList.
     */
    IdList();

    /**
     * @brief Gets the count.
     * @return The count.
     */
    size_t GetCount() const;

    /**
     * @brief Adds an ID.
     * @param id The ID.
     */
    void Add(uint64_t id);

    /**
     * @brief Removes an ID.
     * @param id The ID.
     * @return True if the ID was removed, false otherwise.
     */
    bool Remove(uint64_t id);

    /**
     * @brief Checks if the list contains an ID.
     * @param id The ID.
     * @return True if the list contains the ID, false otherwise.
     */
    bool Contains(uint64_t id) const;

    /**
     * @brief Gets the IDs.
     * @return The IDs.
     */
    const std::vector<uint64_t>& GetIds() const;

    /**
     * @brief Converts the list to a stack item.
     * @return The stack item.
     */
    std::shared_ptr<vm::StackItem> ToStackItem() const;

    /**
     * @brief Initializes the list from a stack item.
     * @param item The stack item.
     */
    void FromStackItem(const std::shared_ptr<vm::StackItem>& item);

    /**
     * @brief Serializes the list to a binary writer.
     * @param writer The binary writer.
     */
    void Serialize(io::BinaryWriter& writer) const;

    /**
     * @brief Deserializes the list from a binary reader.
     * @param reader The binary reader.
     */
    void Deserialize(io::BinaryReader& reader);

  private:
    std::vector<uint64_t> ids_;
};
}  // namespace neo::smartcontract::native
