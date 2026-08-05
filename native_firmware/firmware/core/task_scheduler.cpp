#include "task_scheduler.hpp"

#include <algorithm>

namespace shaer {

int TaskScheduler::schedule(Event event, std::chrono::milliseconds delay) {
    tasks_.push_back({std::move(event), delay, std::chrono::milliseconds(0), false, false});
    return static_cast<int>(tasks_.size() - 1);
}

int TaskScheduler::schedule_periodic(Event event, std::chrono::milliseconds interval) {
    tasks_.push_back({std::move(event), interval, interval, true, false});
    return static_cast<int>(tasks_.size() - 1);
}

void TaskScheduler::cancel(int id) {
    if (id < 0 || static_cast<size_t>(id) >= tasks_.size()) return;
    tasks_[static_cast<size_t>(id)].cancelled = true;
}

void TaskScheduler::update(EventBus& bus, std::chrono::milliseconds delta) {
    for (auto& task : tasks_) {
        if (task.cancelled) continue;
        task.due_in -= delta;
        if (task.due_in.count() > 0) continue;
        bus.publish(task.event);
        if (task.periodic) {
            task.due_in = task.interval;
        } else {
            task.cancelled = true;
        }
    }
    tasks_.erase(
        std::remove_if(tasks_.begin(), tasks_.end(), [](const ScheduledTask& task) {
            return task.cancelled;
        }),
        tasks_.end());
}

size_t TaskScheduler::pending_count() const {
    return tasks_.size();
}

}  // namespace shaer
