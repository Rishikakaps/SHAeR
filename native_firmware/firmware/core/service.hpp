#pragma once

#include "app_state.hpp"
#include "event_bus.hpp"
#include "structured_logger.hpp"

#include <chrono>

namespace shaer {

struct ServiceContext {
    EventBus& events;
    AppStateStore& state;
    StructuredLogger* logger = nullptr;
};

class FirmwareService {
public:
    virtual ~FirmwareService() = default;
    virtual const char* name() const = 0;
    virtual void init(ServiceContext& context) = 0;
    virtual void start(ServiceContext& context) = 0;
    virtual void handle_event(const Event& event, ServiceContext& context) = 0;
    virtual void update(ServiceContext& context, std::chrono::milliseconds delta) = 0;
    virtual void shutdown(ServiceContext& context) = 0;
};

}  // namespace shaer
