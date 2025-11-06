/**
 * @file witness_rule.h
 * @brief Witness Rule
 * @author Neo C++ Team
 * @date 2025
 * @copyright MIT License
 */

#pragma once

#include <neo/cryptography/ecc/ecpoint.h>
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>
#include <neo/io/ijson_serializable.h>
#include <neo/io/iserializable.h>
#include <neo/io/json_reader.h>
#include <neo/io/json_writer.h>
#include <neo/io/uint160.h>

#include <memory>
#include <string>
#include <vector>

namespace neo::smartcontract
{
class ApplicationEngine;
}

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

// Forward declarations
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

    /**
     * @brief Evaluates whether the rule matches for the provided engine state.
     */
    bool Matches(const smartcontract::ApplicationEngine& engine) const;

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
    static constexpr uint8_t MaxNestingDepth = 3;

    /**
     * @brief Maximum number of subitems allowed inside composite conditions.
     */
    static constexpr uint8_t MaxSubitems = 16;

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
     * @brief Serializes the condition including the type discriminator.
     */
    void Serialize(io::BinaryWriter& writer) const override;

    /**
     * @brief Deserializes the condition including the type discriminator.
     */
    void Deserialize(io::BinaryReader& reader) override;

    /**
     * @brief Serializes the condition to JSON.
     */
    void SerializeJson(io::JsonWriter& writer) const override;

    /**
     * @brief Deserializes the condition from JSON.
     */
    void DeserializeJson(const io::JsonReader& reader) override;

    /**
     * @brief Gets the condition type.
     */
    virtual Type GetType() const = 0;

    /**
     * @brief Evaluates the condition against the engine state.
     */
    virtual bool Match(const smartcontract::ApplicationEngine& engine) const = 0;

    /**
     * @brief Creates a condition from the binary stream.
     */
    static std::shared_ptr<WitnessCondition> DeserializeFrom(io::BinaryReader& reader, uint8_t maxDepth);

    /**
     * @brief Creates a condition from JSON.
     */
    static std::shared_ptr<WitnessCondition> FromJson(const io::JsonReader& reader, uint8_t maxDepth);

   protected:
    /**
     * @brief Write the payload without the discriminator byte.
     */
    virtual void SerializeWithoutType(io::BinaryWriter& writer) const = 0;

    /**
     * @brief Read the payload without the discriminator byte.
     */
    virtual void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) = 0;

    /**
     * @brief Parse JSON payload without the discriminator.
     */
    virtual void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth);

    /**
     * @brief Serialize a list of nested conditions.
     */
    static void SerializeConditionArray(io::BinaryWriter& writer,
                                        const std::vector<std::shared_ptr<WitnessCondition>>& conditions);

    /**
     * @brief Deserialize a list of nested conditions.
     */
    static std::vector<std::shared_ptr<WitnessCondition>> DeserializeConditionArray(io::BinaryReader& reader,
                                                                                    uint8_t maxDepth);

    /**
     * @brief Write a list of nested conditions to JSON.
     */
    static void WriteConditionArray(io::JsonWriter& writer, const std::string& key,
                                    const std::vector<std::shared_ptr<WitnessCondition>>& conditions);

    /**
     * @brief Parse a list of nested conditions from JSON.
     */
    static std::vector<std::shared_ptr<WitnessCondition>> ParseConditionArray(const io::JsonReader& reader,
                                                                              const std::string& key, uint8_t maxDepth);

    /**
     * @brief Allow derived types to serialize extra JSON fields.
     */
    virtual void WriteJsonFields(io::JsonWriter& writer) const;
};

/**
 * @brief Boolean condition (constant true/false).
 */
class BooleanCondition : public WitnessCondition
{
   public:
    BooleanCondition() = default;
    explicit BooleanCondition(bool value);

    Type GetType() const override { return Type::Boolean; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;
    bool GetValue() const { return value_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    bool value_{false};
};

/**
 * @brief Logical NOT condition.
 */
class NotCondition : public WitnessCondition
{
   public:
    NotCondition();
    explicit NotCondition(std::shared_ptr<WitnessCondition> condition);

    Type GetType() const override { return Type::Not; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;
    std::shared_ptr<WitnessCondition> GetCondition() const { return condition_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    std::shared_ptr<WitnessCondition> condition_;
};

/**
 * @brief Logical AND condition.
 */
class AndCondition : public WitnessCondition
{
   public:
    AndCondition() = default;

    Type GetType() const override { return Type::And; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

    void SetConditions(std::vector<std::shared_ptr<WitnessCondition>> conditions);
    const std::vector<std::shared_ptr<WitnessCondition>>& GetConditions() const { return conditions_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    std::vector<std::shared_ptr<WitnessCondition>> conditions_;
};

/**
 * @brief Logical OR condition.
 */
class OrCondition : public WitnessCondition
{
   public:
    OrCondition() = default;

    Type GetType() const override { return Type::Or; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

    void SetConditions(std::vector<std::shared_ptr<WitnessCondition>> conditions);
    const std::vector<std::shared_ptr<WitnessCondition>>& GetConditions() const { return conditions_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    std::vector<std::shared_ptr<WitnessCondition>> conditions_;
};

/**
 * @brief Condition matching a specific script hash.
 */
class ScriptHashCondition : public WitnessCondition
{
   public:
    ScriptHashCondition();
    explicit ScriptHashCondition(const io::UInt160& hash);

    Type GetType() const override { return Type::ScriptHash; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

    const io::UInt160& GetHash() const { return hash_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    io::UInt160 hash_;
};

/**
 * @brief Condition matching a manifest group public key.
 */
class GroupCondition : public WitnessCondition
{
   public:
    GroupCondition();
    explicit GroupCondition(const cryptography::ecc::ECPoint& group);

    Type GetType() const override { return Type::Group; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

    const cryptography::ecc::ECPoint& GetGroup() const { return group_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    cryptography::ecc::ECPoint group_;
};

/**
 * @brief Condition indicating the invocation came from the entry point.
 */
class CalledByEntryCondition : public WitnessCondition
{
   public:
    CalledByEntryCondition() = default;

    Type GetType() const override { return Type::CalledByEntry; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
};

/**
 * @brief Condition restricting calls to a specific contract.
 */
class CalledByContractCondition : public WitnessCondition
{
   public:
    CalledByContractCondition();
    explicit CalledByContractCondition(const io::UInt160& hash);

    Type GetType() const override { return Type::CalledByContract; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

    const io::UInt160& GetHash() const { return hash_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    io::UInt160 hash_;
};

/**
 * @brief Condition restricting calls to a specific manifest group.
 */
class CalledByGroupCondition : public WitnessCondition
{
   public:
    CalledByGroupCondition();
    explicit CalledByGroupCondition(const cryptography::ecc::ECPoint& group);

    Type GetType() const override { return Type::CalledByGroup; }
    bool Match(const smartcontract::ApplicationEngine& engine) const override;

    const cryptography::ecc::ECPoint& GetGroup() const { return group_; }

   protected:
    void SerializeWithoutType(io::BinaryWriter& writer) const override;
    void DeserializeWithoutType(io::BinaryReader& reader, uint8_t maxDepth) override;
    void ParseJsonInternal(const io::JsonReader& reader, uint8_t maxDepth) override;
    void WriteJsonFields(io::JsonWriter& writer) const override;

   private:
    cryptography::ecc::ECPoint group_;
};

}  // namespace neo::ledger
