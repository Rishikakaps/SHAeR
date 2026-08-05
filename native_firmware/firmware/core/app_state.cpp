#include "app_state.hpp"

#include <algorithm>
#include <utility>

namespace shaer {

const AppState& AppStateStore::state() const {
    return state_;
}

AppState AppStateStore::snapshot() const {
    return state_;
}

void AppStateStore::apply_boot_settings(const RuntimeSettings& settings) {
    state_.settings = settings;
    if (state_.settings.values.empty()) {
        state_.settings.values["appearance.theme"] = settings.active_theme;
        state_.settings.values["audio.volume"] = std::to_string(settings.volume);
        state_.settings.values["audio.crossfade_seconds"] = std::to_string(settings.crossfade_seconds);
        state_.settings.values["audio.replaygain"] = settings.replaygain_mode;
        state_.settings.values["audio.quality"] = settings.quality_mode;
        state_.settings.values["power.mode"] = settings.power_mode;
    }
    state_.active_theme = settings.active_theme;
    state_.settings.volume = std::clamp(settings.volume, 0, 100);
    state_.audio.crossfade_seconds = std::clamp(settings.crossfade_seconds, 0, 12);
    if (settings.replaygain_mode == "track") {
        state_.audio.replaygain_mode = ReplayGainMode::Track;
    } else if (settings.replaygain_mode == "album") {
        state_.audio.replaygain_mode = ReplayGainMode::Album;
    } else {
        state_.audio.replaygain_mode = ReplayGainMode::Off;
    }
    state_.audio.quality_mode = settings.quality_mode == "archive" ? QualityMode::ArchiveQuality : QualityMode::Balanced;
    state_.dirty = true;
}

void AppStateStore::set_running(bool running) {
    state_.running = running;
}

void AppStateStore::set_reboot_requested(bool reboot_requested) {
    state_.reboot_requested = reboot_requested;
    state_.dirty = true;
}

void AppStateStore::set_tick_count(int tick) {
    state_.tick_count = tick;
}

void AppStateStore::set_dirty(bool dirty) {
    state_.dirty = dirty;
}

void AppStateStore::mark_dirty() {
    state_.dirty = true;
}

void AppStateStore::set_screen(Screen screen, FirmwareState firmware_state, bool push_navigation) {
    state_.previous_screen = state_.current_screen;
    state_.current_screen = screen;
    state_.firmware_state = firmware_state;
    state_.selected_index = 0;
    if (screen != Screen::Settings) {
        state_.settings_ui.inside_category = false;
    }
    if (push_navigation) {
        if (state_.navigation_stack.empty() || state_.navigation_stack.back() != screen) {
            if (state_.navigation_stack.size() >= 8) {
                state_.navigation_stack.pop_front();
            }
            state_.navigation_stack.push_back(screen);
        }
    }
    state_.dirty = true;
}

void AppStateStore::reset_to_screen(Screen screen, FirmwareState firmware_state) {
    state_.navigation_stack.clear();
    state_.navigation_stack.push_back(screen);
    set_screen(screen, firmware_state, false);
}

void AppStateStore::navigate_back() {
    if (state_.navigation_stack.size() > 1) {
        state_.previous_screen = state_.current_screen;
        state_.navigation_stack.pop_back();
        state_.current_screen = state_.navigation_stack.back();
        state_.firmware_state = firmware_state_for_screen(state_.current_screen);
        state_.selected_index = 0;
        state_.dirty = true;
        return;
    }
    reset_to_screen(Screen::Home, FirmwareState::Home);
}

void AppStateStore::set_selected_index(int selected_index) {
    state_.selected_index = std::clamp(selected_index, 0, selectable_count_for(state_) - 1);
    state_.dirty = true;
}

void AppStateStore::move_selection(int delta) {
    if (state_.current_screen == Screen::Settings) {
        move_settings_selection(delta);
        return;
    }
    set_selected_index(state_.selected_index + delta);
}

void AppStateStore::set_notification(Notification notification) {
    state_.notification = std::move(notification);
    state_.dirty = true;
}

void AppStateStore::clear_notification() {
    state_.notification = {};
    state_.dirty = true;
}

void AppStateStore::set_battery(int percent, bool charging) {
    state_.battery_percent = std::clamp(percent, 0, 100);
    state_.charging = charging;
    state_.dirty = true;
}

void AppStateStore::set_bluetooth(bool connected) {
    state_.bluetooth_connected = connected;
    state_.dirty = true;
}

void AppStateStore::set_wifi(bool connected, std::string hint) {
    state_.wifi_connected = connected;
    state_.connection_hint = std::move(hint);
    state_.connection = connected ? ConnectionState::Ready : ConnectionState::WifiRecovering;
    state_.dirty = true;
}

void AppStateStore::set_connection(ConnectionState connection, std::string hint) {
    state_.connection = connection;
    state_.connection_hint = std::move(hint);
    state_.dirty = true;
}

void AppStateStore::set_active_theme(std::string theme_id) {
    state_.active_theme = std::move(theme_id);
    state_.settings.active_theme = state_.active_theme;
    state_.dirty = true;
}

void AppStateStore::set_clock(ClockSnapshot clock) {
    if (clock.time_12h == state_.clock.time_12h &&
        clock.date_label == state_.clock.date_label &&
        clock.valid == state_.clock.valid &&
        clock.source == state_.clock.source) {
        return;
    }
    state_.clock = std::move(clock);
    state_.dirty = true;
}

void AppStateStore::set_local_library(std::vector<Track> tracks) {
    state_.local_library = std::move(tracks);
    state_.dirty = true;
}

void AppStateStore::set_library_index(LibraryIndex index) {
    state_.library_index = std::move(index);
    state_.dirty = true;
}

void AppStateStore::set_settings_categories(std::vector<SettingsCategory> categories) {
    state_.settings_ui.categories = std::move(categories);
    state_.settings_ui.category_index = std::clamp(
        state_.settings_ui.category_index,
        0,
        std::max(0, static_cast<int>(state_.settings_ui.categories.size()) - 1));
    if (!state_.settings_ui.categories.empty()) {
        const auto& items = state_.settings_ui.categories[static_cast<size_t>(state_.settings_ui.category_index)].items;
        state_.settings_ui.item_index = std::clamp(
            state_.settings_ui.item_index,
            0,
            std::max(0, static_cast<int>(items.size()) - 1));
    } else {
        state_.settings_ui.item_index = 0;
    }
    state_.selected_index = state_.settings_ui.inside_category
        ? state_.settings_ui.item_index
        : state_.settings_ui.category_index;
    state_.dirty = true;
}

void AppStateStore::open_settings_category() {
    if (state_.settings_ui.categories.empty()) return;
    state_.settings_ui.inside_category = true;
    state_.settings_ui.item_index = 0;
    state_.selected_index = 0;
    state_.dirty = true;
}

void AppStateStore::close_settings_category() {
    state_.settings_ui.inside_category = false;
    state_.selected_index = state_.settings_ui.category_index;
    state_.dirty = true;
}

void AppStateStore::move_settings_selection(int delta) {
    if (state_.settings_ui.categories.empty()) {
        state_.selected_index = 0;
        state_.dirty = true;
        return;
    }
    if (state_.settings_ui.inside_category) {
        const auto& items = state_.settings_ui.categories[static_cast<size_t>(state_.settings_ui.category_index)].items;
        state_.settings_ui.item_index = std::clamp(
            state_.settings_ui.item_index + delta,
            0,
            std::max(0, static_cast<int>(items.size()) - 1));
        state_.selected_index = state_.settings_ui.item_index;
    } else {
        state_.settings_ui.category_index = std::clamp(
            state_.settings_ui.category_index + delta,
            0,
            std::max(0, static_cast<int>(state_.settings_ui.categories.size()) - 1));
        state_.selected_index = state_.settings_ui.category_index;
    }
    state_.dirty = true;
}

std::optional<SettingsItem> AppStateStore::selected_setting_item() const {
    if (state_.settings_ui.categories.empty() || !state_.settings_ui.inside_category) {
        return std::nullopt;
    }
    const int category = std::clamp(
        state_.settings_ui.category_index,
        0,
        std::max(0, static_cast<int>(state_.settings_ui.categories.size()) - 1));
    const auto& items = state_.settings_ui.categories[static_cast<size_t>(category)].items;
    if (items.empty()) return std::nullopt;
    const int item = std::clamp(state_.settings_ui.item_index, 0, static_cast<int>(items.size()) - 1);
    return items[static_cast<size_t>(item)];
}

void AppStateStore::set_setting_value(const std::string& key, const std::string& value) {
    state_.settings.values[key] = value;
    if (key == "appearance.theme") {
        state_.settings.active_theme = value;
        state_.active_theme = value;
    } else if (key == "audio.volume") {
        try { state_.settings.volume = std::stoi(value); } catch (...) {}
    } else if (key == "audio.crossfade_seconds") {
        try { state_.settings.crossfade_seconds = std::stoi(value); } catch (...) {}
        state_.audio.crossfade_seconds = state_.settings.crossfade_seconds;
    } else if (key == "audio.replaygain") {
        state_.settings.replaygain_mode = value;
        if (value == "track") state_.audio.replaygain_mode = ReplayGainMode::Track;
        else if (value == "album") state_.audio.replaygain_mode = ReplayGainMode::Album;
        else state_.audio.replaygain_mode = ReplayGainMode::Off;
    } else if (key == "audio.quality") {
        state_.settings.quality_mode = value;
        state_.audio.quality_mode = value == "archive" ? QualityMode::ArchiveQuality : QualityMode::Balanced;
    } else if (key == "power.mode") {
        state_.settings.power_mode = value;
        state_.power.mode = value == "battery_saver" ? PowerMode::BatterySaver : PowerMode::Normal;
        apply_power_budget("settings");
    }
    state_.dirty = true;
}

void AppStateStore::set_device_info(DeviceInfoSnapshot info) {
    state_.device_info = std::move(info);
    state_.dirty = true;
}

void AppStateStore::set_power_mode(PowerMode mode, std::string reason) {
    state_.power.mode = mode;
    apply_power_budget(std::move(reason));
}

void AppStateStore::apply_power_budget(std::string reason) {
    if (state_.power.mode == PowerMode::Critical || state_.battery_percent <= 15) {
        state_.power.mode = PowerMode::Critical;
        state_.power.display_budget_fps = 8;
        state_.power.max_animated_elements = 1;
        state_.power.wifi_power_save = true;
        state_.power.bluetooth_idle_allowed = false;
        state_.power.reason = std::move(reason);
        state_.dirty = true;
        return;
    }
    if (state_.power.mode == PowerMode::BatterySaver) {
        state_.power.display_budget_fps = 12;
        state_.power.max_animated_elements = 2;
        state_.power.wifi_power_save = true;
        state_.power.bluetooth_idle_allowed = false;
        state_.power.reason = std::move(reason);
        state_.dirty = true;
        return;
    }
    state_.power.display_budget_fps = 60;
    state_.power.max_animated_elements = 6;
    state_.power.wifi_power_save = false;
    state_.power.bluetooth_idle_allowed = true;
    state_.power.reason = "normal";
    state_.dirty = true;
}

void AppStateStore::set_playback(PlaybackSnapshot playback) {
    state_.playback = std::move(playback);
    state_.dirty = true;
}

void AppStateStore::set_volume(int volume) {
    state_.settings.volume = std::clamp(volume, 0, 100);
    state_.dirty = true;
}

void AppStateStore::append_console(const std::string& line) {
    state_.console.push_back(line);
    while (state_.console.size() > 6) {
        state_.console.erase(state_.console.begin());
    }
}

int selectable_count_for(const AppState& state) {
    if (state.current_screen == Screen::Home) return 4;
    if (state.current_screen == Screen::Library) return std::max<int>(1, static_cast<int>(state.local_library.size()));
    if (state.current_screen == Screen::Settings) {
        if (!state.settings_ui.inside_category) {
            return std::max<int>(1, static_cast<int>(state.settings_ui.categories.size()));
        }
        const int category = std::clamp(
            state.settings_ui.category_index,
            0,
            std::max(0, static_cast<int>(state.settings_ui.categories.size()) - 1));
        if (state.settings_ui.categories.empty()) return 1;
        return std::max<int>(1, static_cast<int>(state.settings_ui.categories[static_cast<size_t>(category)].items.size()));
    }
    if (state.current_screen == Screen::VoiceArchive) return 3;
    if (state.current_screen == Screen::BluetoothConnect) return 1;
    return 1;
}

FirmwareState firmware_state_for_screen(Screen screen) {
    switch (screen) {
        case Screen::Boot: return FirmwareState::Booting;
        case Screen::Home: return FirmwareState::Home;
        case Screen::Library: return FirmwareState::LocalLibrary;
        case Screen::NowPlaying: return FirmwareState::Playback;
        case Screen::Marginalia: return FirmwareState::Marginalia;
        case Screen::VoiceArchive: return FirmwareState::LocalLibrary;
        case Screen::BluetoothConnect: return FirmwareState::Home;
        case Screen::Settings: return FirmwareState::Settings;
        case Screen::About: return FirmwareState::Settings;
        case Screen::Popup: return FirmwareState::Popup;
        case Screen::Aod: return FirmwareState::Sleep;
        case Screen::Charging: return FirmwareState::Charging;
        case Screen::Sleep: return FirmwareState::Sleep;
        case Screen::Shutdown: return FirmwareState::Shutdown;
    }
    return FirmwareState::Home;
}

}  // namespace shaer
