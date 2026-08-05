#pragma once

#include "types.hpp"

#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace shaer {

struct AppState {
    FirmwareState firmware_state = FirmwareState::Booting;
    Screen current_screen = Screen::Boot;
    Screen previous_screen = Screen::Home;
    std::deque<Screen> navigation_stack;
    int selected_index = 0;
    int tick_count = 0;
    bool running = true;
    bool dirty = true;
    bool reboot_requested = false;

    RuntimeSettings settings;
    PlaybackSnapshot playback;
    AudioSettings audio;
    PowerProfile power;
    int battery_percent = 100;
    bool charging = false;
    bool wifi_connected = true;
    bool bluetooth_connected = false;
    ConnectionState connection = ConnectionState::Ready;
    std::string connection_hint = "ready";
    std::string active_theme = "archive_dark";
    ClockSnapshot clock;
    Notification notification;
    LibraryIndex library_index;
    SettingsUiState settings_ui;
    DeviceInfoSnapshot device_info;
    std::vector<Track> local_library;
    std::vector<std::string> console;
};

class AppStateStore {
public:
    const AppState& state() const;
    AppState snapshot() const;

    void apply_boot_settings(const RuntimeSettings& settings);
    void set_running(bool running);
    void set_reboot_requested(bool reboot_requested);
    void set_tick_count(int tick);
    void set_dirty(bool dirty);
    void mark_dirty();
    void set_screen(Screen screen, FirmwareState firmware_state, bool push_navigation);
    void reset_to_screen(Screen screen, FirmwareState firmware_state);
    void navigate_back();
    void set_selected_index(int selected_index);
    void move_selection(int delta);
    void set_notification(Notification notification);
    void clear_notification();
    void set_battery(int percent, bool charging);
    void set_bluetooth(bool connected);
    void set_wifi(bool connected, std::string hint);
    void set_connection(ConnectionState connection, std::string hint);
    void set_active_theme(std::string theme_id);
    void set_clock(ClockSnapshot clock);
    void set_local_library(std::vector<Track> tracks);
    void set_library_index(LibraryIndex index);
    void set_settings_categories(std::vector<SettingsCategory> categories);
    void open_settings_category();
    void close_settings_category();
    void move_settings_selection(int delta);
    std::optional<SettingsItem> selected_setting_item() const;
    void set_setting_value(const std::string& key, const std::string& value);
    void set_device_info(DeviceInfoSnapshot info);
    void set_power_mode(PowerMode mode, std::string reason);
    void apply_power_budget(std::string reason);
    void set_playback(PlaybackSnapshot playback);
    void set_volume(int volume);
    void append_console(const std::string& line);

private:
    AppState state_;
};

int selectable_count_for(const AppState& state);
FirmwareState firmware_state_for_screen(Screen screen);

}  // namespace shaer
