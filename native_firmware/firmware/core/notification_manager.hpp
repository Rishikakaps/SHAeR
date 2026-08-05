#pragma once

#include "app_state.hpp"
#include "event_bus.hpp"

namespace shaer {

class NotificationManager {
public:
    void show(AppStateStore& state, const std::string& title, const std::string& body, std::string confirm_action);
    void clear(AppStateStore& state);
    bool active(const AppState& state) const;
};

}  // namespace shaer

