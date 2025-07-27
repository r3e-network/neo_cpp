#include <neo/core/logging.h>
#include <neo/network/p2p/local_node.h>
#include <neo/network/p2p/task_manager.h>

namespace neo::network::p2p
{
// Minimal stub implementations to allow NeoSystem to link

bool LocalNode::Start(const ChannelsConfig&)
{
    LOG_INFO("LocalNode stub Start() - network module disabled");
    return true;
}

void LocalNode::Stop()
{
    LOG_INFO("LocalNode stub Stop() - network module disabled");
}

TaskManager::TaskManager(std::shared_ptr<ledger::Blockchain>, std::shared_ptr<ledger::MemoryPool>)
{
    LOG_INFO("TaskManager stub constructor - network module disabled");
}

TaskManager::~TaskManager()
{
    LOG_INFO("TaskManager stub destructor - network module disabled");
}
}  // namespace neo::network::p2p