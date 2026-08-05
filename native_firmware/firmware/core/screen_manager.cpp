#include "screen_manager.hpp"

#include <algorithm>

namespace shaer {

const std::vector<ScreenRegistration>& ScreenManager::registrations() {
    static const std::vector<ScreenRegistration> screens = {
        {Screen::Boot, "Boot", "240x320-safe-grid", {"logo", "status"}, {Screen::Home}},
        {Screen::Home, "Home", "240x320-safe-grid", {"header", "menu", "footer"}, {Screen::Library, Screen::BluetoothConnect, Screen::VoiceArchive, Screen::Settings}},
        {Screen::Library, "Local Music Library", "240x320-safe-grid", {"header", "list", "scrollbar"}, {Screen::NowPlaying}},
        {Screen::NowPlaying, "Now Playing", "240x320-safe-grid", {"artwork", "metadata", "progress", "controls", "pen"}, {Screen::Marginalia}},
        {Screen::Marginalia, "Marginalia", "240x320-safe-grid", {"song-context", "canvas", "pen-tools", "page-status"}, {Screen::NowPlaying}},
        {Screen::VoiceArchive, "Voice Memo Library", "240x320-safe-grid", {"header", "list", "recorder"}, {Screen::NowPlaying}},
        {Screen::BluetoothConnect, "Bluetooth / Spotify Connect", "240x320-safe-grid", {"status", "device-list", "actions"}, {Screen::Home}},
        {Screen::Settings, "Settings", "240x320-safe-grid", {"category-list", "value-list"}, {Screen::About}},
        {Screen::About, "About", "240x320-safe-grid", {"logo", "version", "storage"}, {Screen::Settings}},
        {Screen::Popup, "Error / Confirmation", "240x320-safe-grid", {"icon", "message", "actions"}, {}},
        {Screen::Aod, "Always-On Display", "240x320-safe-grid", {"clock", "playback-status"}, {}},
        {Screen::Charging, "Charging", "240x320-safe-grid", {"battery", "status"}, {}},
        {Screen::Sleep, "Sleep", "240x320-safe-grid", {"status"}, {}},
        {Screen::Shutdown, "Shutdown", "240x320-safe-grid", {"status"}, {}}
    };
    return screens;
}

RenderModel ScreenManager::build_render_model(const AppState& state) {
    themes_.set_theme(state.active_theme);
    RenderModel model;
    model.os_name = state.settings.os_name;
    model.firmware_state = state.firmware_state;
    model.screen = state.current_screen;
    model.theme = themes_.render_profile();
    model.blueprint = themes_.screen_blueprint(state.current_screen);
    model.animation = themes_.animation_policy();
    model.tick_count = state.tick_count;
    model.animation.target_fps = std::min(model.animation.target_fps, state.power.display_budget_fps);
    model.animation.max_animated_elements = std::min(model.animation.max_animated_elements, state.power.max_animated_elements);
    model.animation.reduce_motion = state.power.mode != PowerMode::Normal;
    model.battery_percent = state.battery_percent;
    model.charging = state.charging;
    model.wifi_connected = state.wifi_connected;
    model.bluetooth_connected = state.bluetooth_connected;
    model.playback = state.playback;
    model.library_index = state.library_index;
    model.local_library = state.local_library;
    model.selected_index = state.selected_index;
    model.connection_hint = state.connection_hint;
    model.connection_state = state.connection;
    model.audio_settings = state.audio;
    model.power = state.power;
    model.clock = state.clock;
    model.settings_ui = state.settings_ui;
    model.device_info = state.device_info;
    model.volume = state.settings.volume;
    model.popup = state.notification;
    model.console = state.console;
    return model;
}

}  // namespace shaer
