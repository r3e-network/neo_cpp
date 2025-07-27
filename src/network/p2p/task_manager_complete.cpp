#include <neo/core/logging.h>
#include <neo/network/p2p/task_manager.h>

namespace neo::network::p2p
{
TaskManager::TaskManager(std::shared_ptr<ledger::Blockchain> blockchain, std::shared_ptr<ledger::MemoryPool> mempool)
    : blockchain_(blockchain), mempool_(mempool), running_(false)
{
    LOG_INFO("TaskManager initialized");
}

TaskManager::~TaskManager()
{
    Stop();
}

void TaskManager::Start()
{
    if (running_.exchange(true))
        return;

    LOG_INFO("Starting TaskManager");

    // Start task processing threads
    for (int i = 0; i < 2; ++i)
    {
        workers_.emplace_back(&TaskManager::WorkerThread, this);
    }
}

void TaskManager::Stop()
{
    if (!running_.exchange(false))
        return;

    LOG_INFO("Stopping TaskManager");

    // Notify all workers to stop
    cv_.notify_all();

    // Wait for workers to finish
    for (auto& worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    workers_.clear();
}

void TaskManager::ScheduleTask(std::function<void()> task)
{
    if (!running_)
        return;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void TaskManager::WorkerThread()
{
    LOG_DEBUG("TaskManager worker thread started");

    while (running_)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !tasks_.empty() || !running_; });

        while (!tasks_.empty())
        {
            auto task = std::move(tasks_.front());
            tasks_.pop();
            lock.unlock();

            try
            {
                task();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Task execution failed: {}", e.what());
            }

            lock.lock();
        }
    }

    LOG_DEBUG("TaskManager worker thread stopped");
}
}  // namespace neo::network::p2p