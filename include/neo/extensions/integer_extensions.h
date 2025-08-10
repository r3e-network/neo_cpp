#pragma once

#include <cstdint>
#include <vector>

namespace neo::extensions
{
/**
 * @brief Extension methods for integers.
 */
class IntegerExtensions
{
   public:
    /**
     * @brief Gets the size of variable-length of the data.
     * @param value The length of the data.
     * @return The size of variable-length of the data.
     */
    static uint8_t GetVarSize(int32_t value);

    /**
     * @brief Gets the size of variable-length of the data.
     * @param value The length of the data.
     * @return The size of variable-length of the data.
     */
    static uint8_t GetVarSize(uint16_t value);

    /**
     * @brief Gets the size of variable-length of the data.
     * @param value The length of the data.
     * @return The size of variable-length of the data.
     */
    static uint8_t GetVarSize(uint32_t value);

    /**
     * @brief Gets the size of variable-length of the data.
     * @param value The length of the data.
     * @return The size of variable-length of the data.
     */
    static uint8_t GetVarSize(int64_t value);

    /**
     * @brief Gets the size of variable-length of the data.
     * @param value The length of the data.
     * @return The size of variable-length of the data.
     */
    static uint8_t GetVarSize(uint64_t value);

    /**
     * @brief Converts an integer to little-endian byte array.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    static std::vector<uint8_t> ToLittleEndianBytes(int16_t value);

    /**
     * @brief Converts an integer to little-endian byte array.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    static std::vector<uint8_t> ToLittleEndianBytes(uint16_t value);

    /**
     * @brief Converts an integer to little-endian byte array.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    static std::vector<uint8_t> ToLittleEndianBytes(int32_t value);

    /**
     * @brief Converts an integer to little-endian byte array.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    static std::vector<uint8_t> ToLittleEndianBytes(uint32_t value);

    /**
     * @brief Converts an integer to little-endian byte array.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    static std::vector<uint8_t> ToLittleEndianBytes(int64_t value);

    /**
     * @brief Converts an integer to little-endian byte array.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    static std::vector<uint8_t> ToLittleEndianBytes(uint64_t value);

    /**
     * @brief Converts a little-endian byte array to int16.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     * @throws std::out_of_range if there are insufficient bytes.
     */
    static int16_t FromLittleEndianBytes16(const std::vector<uint8_t>& bytes, size_t offset = 0);

    /**
     * @brief Converts a little-endian byte array to uint16.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     * @throws std::out_of_range if there are insufficient bytes.
     */
    static uint16_t FromLittleEndianBytesU16(const std::vector<uint8_t>& bytes, size_t offset = 0);

    /**
     * @brief Converts a little-endian byte array to int32.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     * @throws std::out_of_range if there are insufficient bytes.
     */
    static int32_t FromLittleEndianBytes32(const std::vector<uint8_t>& bytes, size_t offset = 0);

    /**
     * @brief Converts a little-endian byte array to uint32.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     * @throws std::out_of_range if there are insufficient bytes.
     */
    static uint32_t FromLittleEndianBytesU32(const std::vector<uint8_t>& bytes, size_t offset = 0);

    /**
     * @brief Converts a little-endian byte array to int64.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     * @throws std::out_of_range if there are insufficient bytes.
     */
    static int64_t FromLittleEndianBytes64(const std::vector<uint8_t>& bytes, size_t offset = 0);

    /**
     * @brief Converts a little-endian byte array to uint64.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     * @throws std::out_of_range if there are insufficient bytes.
     */
    static uint64_t FromLittleEndianBytesU64(const std::vector<uint8_t>& bytes, size_t offset = 0);

    /**
     * @brief Checks if the system is little-endian.
     * @return True if the system is little-endian, false otherwise.
     */
    static bool IsLittleEndian();

   private:
    /**
     * @brief Template function to convert integer to little-endian bytes.
     * @tparam T The integer type.
     * @param value The integer value.
     * @return The byte array in little-endian format.
     */
    template <typename T>
    static std::vector<uint8_t> ToLittleEndianBytesImpl(T value);

    /**
     * @brief Template function to convert little-endian bytes to integer.
     * @tparam T The integer type.
     * @param bytes The byte array.
     * @param offset The offset in the array.
     * @return The converted integer.
     */
    template <typename T>
    static T FromLittleEndianBytesImpl(const std::vector<uint8_t>& bytes, size_t offset);
};
}  // namespace neo::extensions
