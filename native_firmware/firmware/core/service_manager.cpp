#include "service_manager.hpp"

namespace shaer {

void ServiceManager::add(std::unique_ptr<FirmwareService> service, std::chrono::milliseconds update_interval) {
    services_.push_back({std::move(service), update_interval, std::chrono::milliseconds(0), false});
}

void ServiceManager::init_all(ServiceContext& context) {
    for (auto& registration : services_) {
        registration.service->init(context);
        context.events.publish({EventType::ServiceStarted, 0, 0, InputAction::None, Screen::Home, registration.service->name(), "service initialized"});
    }
}

void ServiceManager::start_all(ServiceContext& context) {
    for (auto& registration : services_) {
        if (registration.started) continue;
        registration.service->start(context);
        registration.started = true;
        context.events.publish({EventType::ServiceStarted, 0, 0, InputAction::None, Screen::Home, registration.service->name(), "service started"});
    }
}

void ServiceManager::dispatch(const Event& event, ServiceContext& context) {
    for (auto& registration : services_) {
        registration.service->handle_event(event, context);
    }
}

void ServiceManager::update_due(ServiceContext& context, std::chrono::milliseconds delta) {
    for (auto& registration : services_) {
        registration.elapsed += delta;
        if (registration.elapsed < registration.update_interval) {
            continue;
        }
        const auto elapsed = registration.elapsed;
        registration.elapsed = std::chrono::milliseconds(0);
        registration.service->update(context, elapsed);
    }
}

void ServiceManager::update_all(ServiceContext& context, std::chrono::milliseconds delta) {
    for (auto& registration : services_) {
        registration.service->update(context, delta);
    }
}

void ServiceManager::shutdown_all(ServiceContext& context) {
    for (auto it = services_.rbegin(); it != services_.rend(); ++it) {
        it->service->shutdown(context);
        context.events.publish({EventType::ServiceStopped, 0, 0, InputAction::None, Screen::Home, it->service->name(), "service stopped"});
    }
}

size_t ServiceManager::service_count() const {
    return services_.size();
}

}  // namespace shaer

