// Additional helper for IO tests
#include <neo/io/binary_reader.h>
#include <neo/io/binary_writer.h>

namespace neo::io::test {
    void FixEndianness(std::vector<uint8_t>& data) {
        // Ensure consistent endianness across platforms
        if constexpr (std::endian::native == std::endian::big) {
            std::reverse(data.begin(), data.end());
        }
    }
    
    bool ValidateSerializedData(const std::vector<uint8_t>& data) {
        // Add validation logic
        return !data.empty() && data.size() % 4 == 0;
    }
}
