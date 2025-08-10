#pragma once

#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>

#include <memory>

namespace neo::ledger
{
/**
 * @brief Witness rule action enum
 */
enum class WitnessRuleAction : uint8_t
{
    Deny = 0x00,
    Allow = 0x01
};

// Forward declaration
class WitnessCondition;

/**
 * @brief Represents a witness rule used to describe the scope of the witness
 */
class WitnessRule : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Default constructor
     */
    WitnessRule();

    /**
     * @brief Constructor with action and condition
     */
    WitnessRule(WitnessRuleAction action, std::shared_ptr<WitnessCondition> condition);

    /**
     * @brief Virtual destructor
     */
    virtual ~WitnessRule() = default;

    /**
     * @brief Gets the action
     */
    WitnessRuleAction GetAction() const { return action_; }

    /**
     * @brief Sets the action
     */
    void SetAction(WitnessRuleAction action) { action_ = action; }

    /**
     * @brief Gets the condition
     */
    std::shared_ptr<WitnessCondition> GetCondition() const { return condition_; }

    /**
     * @brief Sets the condition
     */
    void SetCondition(std::shared_ptr<WitnessCondition> condition) { condition_ = condition; }

    /**
     * @brief Serializes to binary writer
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes from binary reader
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes to JSON
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes from JSON
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Equality operator
     */
    bool operator==(const WitnessRule& other) const;

    /**
     * @brief Inequality operator
     */
    bool operator!=(const WitnessRule& other) const;

   private:
    WitnessRuleAction action_;
    std::shared_ptr<WitnessCondition> condition_;
};

/**
 * @brief Base class for witness conditions
 */
class WitnessCondition : public io::ISerializable, public io::IJsonSerializable
{
   public:
    /**
     * @brief Maximum nesting depth for conditions
     */
    static constexpr uint8_t MaxNestingDepth = 2;

    /**
     * @brief Witness condition type
     */
    enum class Type : uint8_t
    {
        Boolean = 0x00,
        Not = 0x01,
        And = 0x02,
        Or = 0x03,
        ScriptHash = 0x18,
        Group = 0x19,
        CalledByEntry = 0x20,
        CalledByContract = 0x28,
        CalledByGroup = 0x29
    };

    /**
     * @brief Virtual destructor
     */
    virtual ~WitnessCondition() = default;

    /**
     * @brief Gets the condition type
     */
    virtual Type GetType() const = 0;

    /**
     * @brief Creates a condition from binary reader
     */
    static std::shared_ptr<WitnessCondition> DeserializeFrom(io::BinaryReader& reader, uint8_t maxDepth);
};

}  // namespace neo::ledger