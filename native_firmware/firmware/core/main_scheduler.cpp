#include "main_scheduler.hpp"

namespace shaer {

MainScheduler::MainScheduler(std::chrono::milliseconds tick_interval)
    : tick_interval_(tick_interval) {}

void MainScheduler::add_service(std::unique_ptr<FirmwareService> service, std::chrono::milliseconds update_interval) {
    services_.add(std::move(service), update_interval);
}

void MainScheduler::init(ServiceContext& context) {
    services_.init_all(context);
    services_.start_all(context);
}

void MainScheduler::tick(ServiceContext& context) {
    ++tick_count_;
    context.state.set_tick_count(tick_count_);
    context.events.publish({EventType::Tick, 0, tick_count_});
    tasks_.update(context.events, tick_interval_);

    auto dispatch_events = [&]() {
        for (int round = 0; round < 4; ++round) {
            const auto events = context.events.drain();
            if (events.empty()) {
                break;
            }
            for (const auto& event : events) {
                services_.dispatch(event, context);
            }
        }
    };

    dispatch_events();

    services_.update_due(context, tick_interval_);

    dispatch_events();

    services_.update_all(context, std::chrono::milliseconds(0));

    for (int round = 0; round < 2; ++round) {
        const auto events = context.events.drain();
        if (events.empty()) {
            break;
        }
        for (const auto& event : events) {
            services_.dispatch(event, context);
        }
    }
}

void MainScheduler::shutdown(ServiceContext& context) {
    services_.shutdown_all(context);
}

int MainScheduler::tick_count() const {
    return tick_count_;
}

std::chrono::milliseconds MainScheduler::tick_interval() const {
    return tick_interval_;
}

TaskScheduler& MainScheduler::tasks() {
    return tasks_;
}

}  // namespace shaer
