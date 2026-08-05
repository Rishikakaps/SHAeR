#pragma once

#include "event_bus.hpp"

#include <chrono>
#include <vector>

namespace shaer {

struct ScheduledTask {
    Event event;
    std::chrono::milliseconds due_in{0};
    std::chrono::milliseconds interval{0};
    bool periodic = false;
    bool cancelled = false;
};

class TaskScheduler {
public:
    int schedule(Event event, std::chrono::milliseconds delay);
    int schedule_periodic(Event event, std::chrono::milliseconds interval);
    void cancel(int id);
    void update(EventBus& bus, std::chrono::milliseconds delta);
    size_t pending_count() const;

private:
    std::vector<ScheduledTask> tasks_;
};

}  // namespace shaer

