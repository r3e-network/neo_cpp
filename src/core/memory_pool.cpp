#include <neo/core/logging.h>
#include <neo/core/memory_pool.h>

#include <sstream>

namespace neo::core
{
void MemoryPoolManager::report_statistics() const
{
    std::stringstream ss;
    ss << "Memory Pool Statistics:\n";

    auto [allocated, reused, pooled] = byte_vector_pool_.get_stats();
    ss << "  ByteVector Pool - Allocated: " << allocated << ", Reused: " << reused << ", Pooled: " << pooled
       << ", Reuse Rate: " << (reused * 100.0 / (allocated + reused)) << "%\n";

    LOG_INFO("{}", ss.str());
}
}  // namespace neo::core