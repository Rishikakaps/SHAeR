#include "notification_manager.hpp"

#include <utility>

namespace shaer {

void NotificationManager::show(AppStateStore& state, const std::string& title, const std::string& body, std::string confirm_action) {
    state.set_notification({title, body, true, std::move(confirm_action)});
    state.set_screen(Screen::Popup, FirmwareState::Popup, false);
}

void NotificationManager::clear(AppStateStore& state) {
    state.clear_notification();
}

bool NotificationManager::active(const AppState& state) const {
    return !state.notification.title.empty();
}

}  // namespace shaer

