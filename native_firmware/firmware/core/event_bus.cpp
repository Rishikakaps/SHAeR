#include "event_bus.hpp"

namespace shaer {

void EventBus::publish(Event event) {
    event.sequence = sequence_++;
    queue_.push_back(std::move(event));
}

std::vector<Event> EventBus::drain() {
    std::vector<Event> events;
    events.reserve(queue_.size());
    while (!queue_.empty()) {
        events.push_back(std::move(queue_.front()));
        queue_.pop_front();
    }
    return events;
}

bool EventBus::empty() const {
    return queue_.empty();
}

uint64_t EventBus::next_sequence() const {
    return sequence_;
}

const char* to_string(EventType type) {
    switch (type) {
        case EventType::BootStarted: return "BOOT_STARTED";
        case EventType::BootCompleted: return "BOOT_COMPLETED";
        case EventType::BootDiagnosticRaised: return "BOOT_DIAGNOSTIC_RAISED";
        case EventType::Tick: return "TICK";
        case EventType::InputReceived: return "INPUT_RECEIVED";
        case EventType::ButtonOkPressed: return "BUTTON_OK_PRESSED";
        case EventType::ButtonBackPressed: return "BUTTON_BACK_PRESSED";
        case EventType::NavigateUp: return "NAVIGATE_UP";
        case EventType::NavigateDown: return "NAVIGATE_DOWN";
        case EventType::PlayPausePressed: return "PLAY_PAUSE_PRESSED";
        case EventType::OpenLibraryRequested: return "OPEN_LIBRARY_REQUESTED";
        case EventType::OpenSettingsRequested: return "OPEN_SETTINGS_REQUESTED";
        case EventType::SettingActivated: return "SETTING_ACTIVATED";
        case EventType::SettingsChanged: return "SETTINGS_CHANGED";
        case EventType::BatterySaverToggleRequested: return "BATTERY_SAVER_TOGGLE_REQUESTED";
        case EventType::CycleThemeRequested: return "CYCLE_THEME_REQUESTED";
        case EventType::SleepRequested: return "SLEEP_REQUESTED";
        case EventType::WakeRequested: return "WAKE_REQUESTED";
        case EventType::ShutdownRequested: return "SHUTDOWN_REQUESTED";
        case EventType::RebootRequested: return "REBOOT_REQUESTED";
        case EventType::LocalLibraryScanRequested: return "LOCAL_LIBRARY_SCAN_REQUESTED";
        case EventType::LocalLibraryLoaded: return "LOCAL_LIBRARY_LOADED";
        case EventType::PlaySelectedTrackRequested: return "PLAY_SELECTED_TRACK_REQUESTED";
        case EventType::SpotifyConnectRequested: return "SPOTIFY_CONNECT_REQUESTED";
        case EventType::SpotifyConnectionLost: return "SPOTIFY_CONNECTION_LOST";
        case EventType::BluetoothConnectionLost: return "BLUETOOTH_CONNECTION_LOST";
        case EventType::WifiConnectionLost: return "WIFI_CONNECTION_LOST";
        case EventType::LowBatterySimulated: return "LOW_BATTERY_SIMULATED";
        case EventType::PlaybackStarted: return "PLAYBACK_STARTED";
        case EventType::PlaybackPaused: return "PLAYBACK_PAUSED";
        case EventType::PlaybackStopped: return "PLAYBACK_STOPPED";
        case EventType::NextTrackRequested: return "NEXT_TRACK_REQUESTED";
        case EventType::PreviousTrackRequested: return "PREVIOUS_TRACK_REQUESTED";
        case EventType::SeekForwardRequested: return "SEEK_FORWARD_REQUESTED";
        case EventType::SeekBackwardRequested: return "SEEK_BACKWARD_REQUESTED";
        case EventType::VolumeUpRequested: return "VOLUME_UP_REQUESTED";
        case EventType::VolumeDownRequested: return "VOLUME_DOWN_REQUESTED";
        case EventType::ClockChanged: return "CLOCK_CHANGED";
        case EventType::ScreenChanged: return "SCREEN_CHANGED";
        case EventType::RenderRequested: return "RENDER_REQUESTED";
        case EventType::ServiceStarted: return "SERVICE_STARTED";
        case EventType::ServiceStopped: return "SERVICE_STOPPED";
        case EventType::QuitRequested: return "QUIT_REQUESTED";
    }
    return "UNKNOWN";
}

}  // namespace shaer
