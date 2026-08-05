#pragma once

#include "hal.hpp"
#include "notebook_store.hpp"
#include "theme_engine.hpp"
#include "types.hpp"

#include <deque>
#include <memory>
#include <string>

namespace shaer {

struct Hardware {
    Display* display = nullptr;
    AudioOutput* audio = nullptr;
    Battery* battery = nullptr;
    Bluetooth* bluetooth = nullptr;
};

class ShaerApp {
public:
    explicit ShaerApp(Hardware hardware);

    void boot();
    void apply_runtime_settings(const RuntimeSettings& settings);
    void show_diagnostic_popup(const std::string& title, const std::string& body);
    void handle(InputAction action);
    void select_theme(const std::string& id);
    bool record_stroke(const NotebookPage::Stroke& stroke);
    void render();

    FirmwareState firmware_state() const;
    Screen screen() const;
    int navigation_depth() const;
    int selected_index() const;
    AnimationPolicy animation_policy() const;
    TransitionPlan transition_plan() const;
    ConnectionState connection_state() const;
    AudioSettings audio_settings() const;
    PowerProfile power_profile() const;
    ThemeRenderProfile theme_profile() const;
    PlaybackState playback_state() const;
    PlaybackSource playback_source() const;
    PlaybackSnapshot playback_snapshot() const;
    const Notebook& notebook() const;
    Notification popup() const;
    const std::deque<std::string>& log() const;

private:
    struct NavigationEntry {
        Screen screen = Screen::Home;
        int selected_index = 0;
        int scroll_offset = 0;
    };

    void open_library();
    void open_now_playing();
    void open_marginalia();
    void open_settings();
    void open_about();
    void navigate_to(Screen screen);
    void reset_navigation_to(Screen screen);
    void set_screen_direct(Screen screen, bool blocks_input, std::string reason);
    void navigate_back();
    bool transition_firmware_state(FirmwareState next, std::string reason);
    bool firmware_transition_allowed(FirmwareState from, FirmwareState to) const;
    FirmwareState state_for_screen(Screen screen) const;
    void toggle_playback();
    void start_local_playback(Track track);
    void start_spotify_playback(Track track);
    void stop_playback(std::string reason);
    void toggle_playback_pause();
    void cycle_theme();
    void activate_selected_setting();
    void cycle_crossfade();
    void cycle_replaygain();
    void cycle_quality();
    void cycle_power_mode();
    void spotify_disconnect();
    void bluetooth_disconnect();
    void low_battery();
    void toggle_battery_saver();
    void enter_sleep();
    void begin_shutdown();
    void confirm_popup();
    void cancel_popup();
    void push_log(std::string message);
    RenderModel model() const;
    int selectable_count_for(Screen screen) const;
    void clamp_selection();

    Hardware hw_;
    ThemeEngine theme_;
    FirmwareState firmware_state_ = FirmwareState::Booting;
    Screen screen_ = Screen::Boot;
    Screen previous_screen_ = Screen::Home;
    std::deque<NavigationEntry> navigation_stack_;
    Notification popup_;
    Notebook notebook_;
    NotebookStore notebook_store_;
    int scroll_offset_ = 0;
    PlaybackSnapshot playback_;
    int selected_index_ = 0;
    int volume_ = 50;
    AudioSettings audio_settings_;
    PowerProfile power_profile_;
    bool wifi_connected_ = true;
    std::string connection_hint_ = "ready";
    ConnectionState connection_state_ = ConnectionState::Ready;
    TransitionPlan transition_;
    int battery_override_percent_ = -1;
    std::deque<std::string> log_;

    int effective_battery_percent() const;
    bool force_power_saving() const;
    void refresh_power_profile(std::string reason);
    void save_notebook(const std::string& reason);
};

}  // namespace shaer
