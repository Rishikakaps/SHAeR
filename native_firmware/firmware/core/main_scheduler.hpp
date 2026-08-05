#pragma once

#include "service_manager.hpp"
#include "task_scheduler.hpp"

#include <chrono>
#include <memory>
#include <vector>

namespace shaer {

class MainScheduler {
public:
    explicit MainScheduler(std::chrono::milliseconds tick_interval = std::chrono::milliseconds(16));

    void add_service(std::unique_ptr<FirmwareService> service, std::chrono::milliseconds update_interval = std::chrono::milliseconds(16));
    void init(ServiceContext& context);
    void tick(ServiceContext& context);
    void shutdown(ServiceContext& context);
    int tick_count() const;
    std::chrono::milliseconds tick_interval() const;
    TaskScheduler& tasks();

private:
    std::chrono::milliseconds tick_interval_;
    ServiceManager services_;
    TaskScheduler tasks_;
    int tick_count_ = 0;
};

}  // namespace shaer
