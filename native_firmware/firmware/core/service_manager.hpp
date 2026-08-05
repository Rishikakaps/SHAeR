#pragma once

#include "service.hpp"

#include <chrono>
#include <memory>
#include <vector>

namespace shaer {

struct ServiceRegistration {
    std::unique_ptr<FirmwareService> service;
    std::chrono::milliseconds update_interval{16};
    std::chrono::milliseconds elapsed{0};
    bool started = false;
};

class ServiceManager {
public:
    void add(std::unique_ptr<FirmwareService> service, std::chrono::milliseconds update_interval);
    void init_all(ServiceContext& context);
    void start_all(ServiceContext& context);
    void dispatch(const Event& event, ServiceContext& context);
    void update_due(ServiceContext& context, std::chrono::milliseconds delta);
    void update_all(ServiceContext& context, std::chrono::milliseconds delta);
    void shutdown_all(ServiceContext& context);
    size_t service_count() const;

private:
    std::vector<ServiceRegistration> services_;
};

}  // namespace shaer

