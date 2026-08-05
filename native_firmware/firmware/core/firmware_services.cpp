#include "firmware_services.hpp"
#include "settings_store.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unistd.h>

namespace shaer {

namespace {

int minutes_from_setting(const std::string& value) {
    if (value == "off" || value.empty()) return 0;
    try {
        return std::stoi(value);
    } catch (...) {
        return 0;
    }
}

}  // namespace

const char* LoggingService::name() const { return "LoggingService"; }
void LoggingService::init(ServiceContext& context) {
    if (context.logger) {
        context.logger->log(LogLevel::Info, "system", "service_init", "logging service ready");
    }
}
void LoggingService::start(ServiceContext&) {}
void LoggingService::handle_event(const Event& event, ServiceContext& context) {
    if (!context.logger || event.type == EventType::Tick) return;
    context.logger->log(LogLevel::Debug, "event", to_string(event.type), event.message, {
        {"source", event.source},
        {"sequence", std::to_string(event.sequence)},
    });
}
void LoggingService::update(ServiceContext&, std::chrono::milliseconds) {}
void LoggingService::shutdown(ServiceContext& context) {
    if (context.logger) {
        context.logger->log(LogLevel::Info, "system", "service_shutdown", "logging service stopped");
    }
}

InputService::InputService(Input& input) : input_(input) {}
const char* InputService::name() const { return "InputService"; }
void InputService::init(ServiceContext&) {}
void InputService::start(ServiceContext&) {}
void InputService::handle_event(const Event&, ServiceContext&) {}
void InputService::update(ServiceContext& context, std::chrono::milliseconds) {
    const InputAction action = input_.poll_action();
    if (action == InputAction::None) return;
    Event event = input_manager_.map(action);
    event.source = name();
    event.message = "input action received";
    context.events.publish(event);
}
void InputService::shutdown(ServiceContext&) {}

const char* NavigationService::name() const { return "NavigationService"; }
void NavigationService::init(ServiceContext& context) {
    context.state.reset_to_screen(Screen::Home, FirmwareState::Home);
    context.state.append_console("[Navigation] Home");
    context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Home, name(), "initial render"});
}
void NavigationService::start(ServiceContext&) {}
void NavigationService::handle_event(const Event& event, ServiceContext& context) {
    const AppState& state = context.state.state();
    switch (event.type) {
        case EventType::ButtonOkPressed:
            if (state.current_screen == Screen::Home) {
                if (state.selected_index == 0) {
                    change_screen(context, Screen::Library, FirmwareState::LocalLibrary);
                } else if (state.selected_index == 1) {
                    change_screen(context, Screen::BluetoothConnect, FirmwareState::Home);
                } else if (state.selected_index == 2) {
                    change_screen(context, Screen::VoiceArchive, FirmwareState::LocalLibrary);
                } else {
                    change_screen(context, Screen::Settings, FirmwareState::Settings);
                }
            } else if (state.current_screen == Screen::BluetoothConnect) {
                context.state.set_notification({"SPOTIFY FOUND", "MAIN PHONE READY", true, "return_previous"});
                context.state.set_screen(Screen::Popup, FirmwareState::Popup, false);
                context.state.append_console("[Spotify] Connect device detected");
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Popup, name(), "spotify device popup"});
            } else if (state.current_screen == Screen::NowPlaying) {
                context.events.publish({EventType::PlayPausePressed, 0, 0, InputAction::PlayPause, Screen::NowPlaying, name(), "now playing button"});
            } else if (state.current_screen == Screen::Popup) {
                const Screen previous = state.previous_screen;
                const std::string action = state.notification.confirm_action;
                notifications_.clear(context.state);
                if (action == "open_library") {
                    change_screen(context, Screen::Library, FirmwareState::LocalLibrary);
                } else if (action == "shutdown") {
                    context.events.publish({EventType::ShutdownRequested, 0, 0, InputAction::Confirm, Screen::Shutdown, name(), "shutdown confirmed"});
                } else if (action == "restart") {
                    context.events.publish({EventType::RebootRequested, 0, 0, InputAction::Confirm, Screen::Shutdown, name(), "restart confirmed"});
                } else {
                    change_screen(context, previous, firmware_state_for_screen(previous));
                }
            } else if (state.current_screen == Screen::Library) {
                context.events.publish({EventType::PlaySelectedTrackRequested, 0, 0, InputAction::Confirm, Screen::Library, name(), "library item selected"});
            } else if (state.current_screen == Screen::Settings) {
                if (!state.settings_ui.inside_category) {
                    context.state.open_settings_category();
                    context.state.append_console("[Settings] category opened");
                    context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Settings, name(), "settings category opened"});
                } else {
                    context.events.publish({EventType::SettingActivated, 0, 0, InputAction::Confirm, Screen::Settings, name(), "setting selected"});
                }
            }
            break;
        case EventType::ButtonBackPressed:
            if (context.state.state().current_screen == Screen::Popup) {
                notifications_.clear(context.state);
                context.state.reset_to_screen(Screen::Home, FirmwareState::Home);
                context.state.append_console("[Dialog] cancelled");
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::Back, Screen::Home, name(), "dialog cancelled"});
                break;
            }
            if (context.state.state().current_screen == Screen::Settings &&
                context.state.state().settings_ui.inside_category) {
                context.state.close_settings_category();
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Settings, name(), "settings category closed"});
                break;
            }
            context.state.navigate_back();
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "back"});
            break;
        case EventType::NavigateUp:
            if (context.state.state().current_screen == Screen::NowPlaying) {
                context.events.publish({EventType::VolumeUpRequested, 0, 0, InputAction::VolumeUp, Screen::NowPlaying, name(), "now playing volume up"});
                break;
            }
            context.state.move_selection(-1);
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "selection changed"});
            break;
        case EventType::NavigateDown:
            if (context.state.state().current_screen == Screen::NowPlaying) {
                context.events.publish({EventType::VolumeDownRequested, 0, 0, InputAction::VolumeDown, Screen::NowPlaying, name(), "now playing volume down"});
                break;
            }
            context.state.move_selection(1);
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "selection changed"});
            break;
        case EventType::OpenLibraryRequested:
            change_screen(context, Screen::Library, FirmwareState::LocalLibrary);
            break;
        case EventType::OpenSettingsRequested:
            change_screen(context, Screen::Settings, FirmwareState::Settings);
            break;
        case EventType::PlaybackStarted:
            change_screen(context, Screen::NowPlaying, FirmwareState::Playback);
            break;
        default:
            break;
    }
}
void NavigationService::update(ServiceContext&, std::chrono::milliseconds) {}
void NavigationService::shutdown(ServiceContext&) {}
void NavigationService::change_screen(ServiceContext& context, Screen screen, FirmwareState state) {
    context.state.set_screen(screen, state, true);
    context.state.append_console(std::string("[Screen] ") + to_string(screen));
    context.events.publish({EventType::ScreenChanged, 0, 0, InputAction::None, screen, name(), "screen changed"});
    context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, screen, name(), "screen changed"});
}

const char* ThemeService::name() const { return "ThemeService"; }
void ThemeService::init(ServiceContext&) {}
void ThemeService::start(ServiceContext&) {}
void ThemeService::handle_event(const Event& event, ServiceContext& context) {
    if (event.type == EventType::SettingsChanged &&
        event.fields.count("key") &&
        event.fields.at("key") == "appearance.theme") {
        context.state.set_active_theme(event.fields.at("value"));
        context.state.append_console(std::string("[Theme] settings -> ") + context.state.state().active_theme);
        context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "theme setting changed"});
        return;
    }
    if (event.type != EventType::CycleThemeRequested) return;
    const auto& current = context.state.state().active_theme;
    auto it = std::find(theme_ids_.begin(), theme_ids_.end(), current);
    if (it == theme_ids_.end() || ++it == theme_ids_.end()) {
        context.state.set_active_theme(theme_ids_.front());
    } else {
        context.state.set_active_theme(*it);
    }
    context.state.append_console(std::string("[Theme] ") + context.state.state().active_theme);
    context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "theme changed"});
}
void ThemeService::update(ServiceContext&, std::chrono::milliseconds) {}
void ThemeService::shutdown(ServiceContext&) {}

const char* SettingsService::name() const { return "SettingsService"; }
void SettingsService::init(ServiceContext& context) {
    refresh_catalog(context);
}
void SettingsService::start(ServiceContext&) {}
void SettingsService::handle_event(const Event& event, ServiceContext& context) {
    switch (event.type) {
        case EventType::SettingActivated: {
            const auto selected = context.state.selected_setting_item();
            if (!selected || !selected->editable) {
                context.state.append_console("[Settings] read-only item");
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Settings, name(), "read-only setting"});
                break;
            }
            if (selected->key == "power.shutdown") {
                context.state.set_notification({"Power off?", "OK shuts down. Hold knob to cancel.", true, "shutdown"});
                context.state.set_screen(Screen::Popup, FirmwareState::Popup, false);
                context.state.append_console("[Power] shutdown confirmation");
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Popup, name(), "shutdown confirmation"});
                break;
            }
            if (selected->key == "power.restart") {
                context.state.set_notification({"Restart?", "OK restarts. Hold knob to cancel.", true, "restart"});
                context.state.set_screen(Screen::Popup, FirmwareState::Popup, false);
                context.state.append_console("[Power] restart confirmation");
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Popup, name(), "restart confirmation"});
                break;
            }
            const std::string next = next_setting_value(selected->key, selected->value);
            context.state.set_setting_value(selected->key, next);
            persist(context.state.state().settings, selected->key, next);
            refresh_catalog(context);
            if (selected->key == "storage.manual_rescan" || selected->key == "storage.rebuild_database") {
                context.events.publish({EventType::LocalLibraryScanRequested, 0, 0, InputAction::None, Screen::Library, name(), "manual library rescan"});
            }
            context.state.append_console("[Settings] " + selected->label + " = " + next);
            context.events.publish({EventType::SettingsChanged, 0, 0, InputAction::Confirm, Screen::Settings, name(), "setting saved", {{"key", selected->key}, {"value", next}}});
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Settings, name(), "setting changed"});
            break;
        }
        case EventType::LocalLibraryLoaded:
        case EventType::ClockChanged:
            refresh_catalog(context);
            break;
        default:
            break;
    }
}
void SettingsService::update(ServiceContext&, std::chrono::milliseconds) {}
void SettingsService::shutdown(ServiceContext&) {}

std::string SettingsService::database_path_for(const RuntimeSettings& settings) const {
#if defined(__APPLE__)
    if (settings.music_directory.rfind("/tmp/", 0) == 0) {
        return (std::filesystem::path(settings.music_directory).parent_path() / "settings.db").string();
    }
    return "data/settings.db";
#else
    if (settings.music_directory.rfind("/var/lib/shaer/", 0) == 0 || settings.music_directory == "/var/lib/shaer/music") {
        return "/var/lib/shaer/settings.db";
    }
    return "data/settings.db";
#endif
}

void SettingsService::refresh_catalog(ServiceContext& context) {
    const auto categories = build_settings_catalog(
        context.state.state().settings,
        context.state.state().device_info,
        context.state.state().battery_percent,
        context.state.state().charging,
        context.state.state().library_index);
    context.state.set_settings_categories(categories);
}

bool SettingsService::persist(const RuntimeSettings& settings, const std::string& key, const std::string& value) const {
    SettingsStore store(database_path_for(settings));
    return store.open() && store.migrate() && store.put(key, value);
}

const char* DeviceInfoService::name() const { return "DeviceInfoService"; }
void DeviceInfoService::init(ServiceContext& context) {
    context.state.set_device_info(read_info(context.state.state()));
}
void DeviceInfoService::start(ServiceContext&) {}
void DeviceInfoService::handle_event(const Event&, ServiceContext&) {}
void DeviceInfoService::update(ServiceContext& context, std::chrono::milliseconds delta) {
    elapsed_ += delta;
    if (elapsed_ < std::chrono::seconds(5)) return;
    elapsed_ = std::chrono::milliseconds(0);
    context.state.set_device_info(read_info(context.state.state()));
}
void DeviceInfoService::shutdown(ServiceContext&) {}

DeviceInfoSnapshot DeviceInfoService::read_info(const AppState& state) const {
    DeviceInfoSnapshot info = state.device_info;
    char hostname[128]{};
    if (gethostname(hostname, sizeof(hostname) - 1) == 0) {
        info.hostname = hostname;
    }
    info.device_name = state.settings.os_name.empty() ? "SHAeR" : state.settings.os_name;
    std::error_code ec;
    const auto space = std::filesystem::space(state.settings.music_directory.empty() ? "." : state.settings.music_directory, ec);
    if (!ec) {
        info.storage_free_mb = static_cast<long long>(space.available / (1024 * 1024));
        info.storage_used_mb = static_cast<long long>((space.capacity - space.available) / (1024 * 1024));
    }
    std::ifstream temp("/sys/class/thermal/thermal_zone0/temp");
    int milli_c = 0;
    if (temp >> milli_c) {
        info.cpu_temp_c = milli_c / 1000;
    }
    std::ifstream mem("/proc/meminfo");
    std::string key;
    long long value = 0;
    std::string unit;
    long long total_kb = 0;
    long long available_kb = 0;
    while (mem >> key >> value >> unit) {
        if (key == "MemTotal:") total_kb = value;
        if (key == "MemAvailable:") available_kb = value;
    }
    if (total_kb > 0 && available_kb > 0) {
        info.memory_used_mb = (total_kb - available_kb) / 1024;
    }
    std::ifstream commit(".git/HEAD");
    std::string head;
    if (std::getline(commit, head)) {
        info.git_commit = head.size() > 12 ? head.substr(0, 12) : head;
    }
    info.ip_address = state.wifi_connected ? "connected" : "offline";
    info.battery_health_percent = 100;
    info.fps = state.power.display_budget_fps;
    return info;
}

const char* LocalLibraryService::name() const { return "LocalLibraryService"; }
void LocalLibraryService::init(ServiceContext& context) {
    context.events.publish({EventType::LocalLibraryScanRequested, 0, 0, InputAction::None, Screen::Library, name(), "initial library scan"});
}
void LocalLibraryService::start(ServiceContext&) {}
void LocalLibraryService::handle_event(const Event& event, ServiceContext& context) {
    if (event.type != EventType::LocalLibraryScanRequested) return;
    const std::string music_dir = context.state.state().settings.music_directory;
    MusicLibraryStore library(database_path_for(music_dir));
    if (!library.open() || !library.migrate()) {
        context.state.append_console("[Library] database unavailable: " + library.last_error());
        context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "library database error"});
        return;
    }
    const auto scan = library.scan(music_dir);
    const auto tracks = library.songs();
    context.state.set_library_index(library.index());
    context.state.set_local_library(tracks);
    context.state.append_console("[Library] " + std::to_string(tracks.size()) + " tracks indexed");
    context.events.publish({EventType::LocalLibraryLoaded, 0, 0, InputAction::None, Screen::Library, name(), "local library loaded", {
        {"tracks", std::to_string(tracks.size())},
        {"seen", std::to_string(scan.files_seen)},
        {"changed", std::to_string(scan.tracks_added_or_updated)},
    }});
    context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "library changed"});
}
void LocalLibraryService::update(ServiceContext&, std::chrono::milliseconds) {}
void LocalLibraryService::shutdown(ServiceContext&) {}

std::string LocalLibraryService::database_path_for(const std::string& directory) const {
#if defined(__APPLE__)
    (void)directory;
    return "data/shaer.db";
#else
    if (directory.rfind("/var/lib/shaer/", 0) == 0 || directory == "/var/lib/shaer/music") {
        return "/var/lib/shaer/shaer.db";
    }
    return "data/shaer.db";
#endif
}

AudioService::AudioService(AudioOutput& audio) : audio_(audio) {}
const char* AudioService::name() const { return "AudioService"; }
void AudioService::init(ServiceContext&) {}
void AudioService::start(ServiceContext& context) {
    audio_.set_volume(context.state.state().settings.volume);
    engine_.set_volume_target(context.state.state().settings.volume);
    engine_.set_crossfade_seconds(context.state.state().audio.crossfade_seconds);
    engine_.set_sleep_timer_minutes(minutes_from_setting(
        context.state.state().settings.values.count("power.sleep_timer")
            ? context.state.state().settings.values.at("power.sleep_timer")
            : "off"));
    audio_.set_crossfade_seconds(context.state.state().audio.crossfade_seconds);
}
void AudioService::handle_event(const Event& event, ServiceContext& context) {
    switch (event.type) {
        case EventType::PlaySelectedTrackRequested: {
            const auto& library = context.state.state().local_library;
            if (library.empty()) {
                context.state.set_notification({"No local music yet", "Sync music from the companion app or set library.music_directory.", true, "open_library"});
                context.state.set_screen(Screen::Popup, FirmwareState::Popup, false);
                context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Popup, name(), "empty library"});
                break;
            }
            const int index = std::clamp(context.state.state().selected_index, 0, static_cast<int>(library.size()) - 1);
            engine_.request_focus("local");
            engine_.set_crossfade_seconds(context.state.state().audio.crossfade_seconds);
            engine_.set_low_power(context.state.state().power.mode != PowerMode::Normal);
            audio_.set_crossfade_seconds(engine_.crossfade_seconds());
            apply_commands(context, engine_.load_queue(library, index));
            context.state.set_connection(ConnectionState::LocalOffline, "local playback");
            context.state.append_console("[Audio] local play: " + engine_.snapshot().track.title);
            context.events.publish({EventType::PlaybackStarted, 0, 0, InputAction::None, Screen::NowPlaying, name(), "local playback started"});
            break;
        }
        case EventType::SpotifyConnectRequested:
            context.state.set_connection(ConnectionState::SpotifyActive, "spotify connect ready");
            context.state.append_console("[Spotify] connect requested");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "spotify connect requested"});
            break;
        case EventType::PlayPausePressed:
            if (engine_.snapshot().state == PlaybackState::Playing) {
                apply_commands(context, engine_.pause());
                context.events.publish({EventType::PlaybackPaused, 0, 0, InputAction::PlayPause, context.state.state().current_screen, name(), "paused"});
            } else if (engine_.snapshot().source != PlaybackSource::None) {
                apply_commands(context, engine_.resume());
                context.events.publish({EventType::PlaybackStarted, 0, 0, InputAction::PlayPause, context.state.state().current_screen, name(), "resumed"});
            }
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "playback toggled"});
            break;
        case EventType::SpotifyConnectionLost:
        case EventType::WifiConnectionLost:
            apply_commands(context, engine_.stop("connectivity loss"));
            context.events.publish({EventType::PlaybackStopped, 0, 0, InputAction::None, context.state.state().current_screen, name(), "connectivity loss stopped playback"});
            break;
        case EventType::NextTrackRequested:
            apply_commands(context, engine_.next());
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::Next, context.state.state().current_screen, name(), "next track"});
            break;
        case EventType::PreviousTrackRequested:
            apply_commands(context, engine_.previous());
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::Previous, context.state.state().current_screen, name(), "previous track"});
            break;
        case EventType::SeekForwardRequested:
            apply_commands(context, engine_.seek_relative(10));
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::SeekForward, context.state.state().current_screen, name(), "seek forward"});
            break;
        case EventType::SeekBackwardRequested:
            apply_commands(context, engine_.seek_relative(-10));
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::SeekBackward, context.state.state().current_screen, name(), "seek backward"});
            break;
        case EventType::VolumeUpRequested:
            context.state.set_volume(context.state.state().settings.volume + 5);
            apply_commands(context, engine_.set_volume_target(context.state.state().settings.volume));
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::VolumeUp, context.state.state().current_screen, name(), "volume up"});
            break;
        case EventType::VolumeDownRequested:
            context.state.set_volume(context.state.state().settings.volume - 5);
            apply_commands(context, engine_.set_volume_target(context.state.state().settings.volume));
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::VolumeDown, context.state.state().current_screen, name(), "volume down"});
            break;
        case EventType::SettingsChanged:
            if (event.fields.count("key") && event.fields.at("key") == "audio.crossfade_seconds") {
                try {
                    engine_.set_crossfade_seconds(std::stoi(event.fields.at("value")));
                    audio_.set_crossfade_seconds(engine_.crossfade_seconds());
                } catch (...) {}
            }
            if (event.fields.count("key") && event.fields.at("key") == "power.sleep_timer") {
                engine_.set_sleep_timer_minutes(minutes_from_setting(event.fields.at("value")));
            }
            break;
        default:
            break;
    }
}
void AudioService::update(ServiceContext& context, std::chrono::milliseconds delta) {
    engine_.set_low_power(context.state.state().power.mode != PowerMode::Normal);
    const auto commands = engine_.update(delta);
    if (!commands.empty()) {
        apply_commands(context, commands);
        context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "playback engine update"});
    }
}
void AudioService::shutdown(ServiceContext& context) {
    apply_commands(context, engine_.stop("service shutdown"));
    if (context.logger) {
        context.logger->log(LogLevel::Info, "audio", "shutdown", "audio output stopped");
    }
}

void AudioService::apply_commands(ServiceContext& context, const std::vector<PlaybackCommand>& commands) {
    for (const auto& command : commands) {
        switch (command.type) {
            case PlaybackCommandType::PlayTrack:
                audio_.set_volume(context.state.state().settings.volume);
                audio_.play_local(command.track);
                break;
            case PlaybackCommandType::PauseOutput:
                audio_.pause();
                break;
            case PlaybackCommandType::StopOutput:
                audio_.stop();
                break;
            case PlaybackCommandType::SetVolume:
                audio_.set_volume(command.value);
                break;
            case PlaybackCommandType::SeekTo:
                audio_.seek_seconds(command.value);
                break;
            case PlaybackCommandType::PrebufferTrack:
                audio_.prebuffer_local(command.track);
                break;
            case PlaybackCommandType::SleepTimerExpired:
                context.state.append_console("[Audio] sleep timer expired");
                break;
            case PlaybackCommandType::None:
                break;
        }
        if (context.logger && command.type != PlaybackCommandType::None) {
            context.logger->log(LogLevel::Debug, "audio", to_string(command.type), command.reason);
        }
    }
    context.state.set_playback(engine_.snapshot());
}

const char* ConnectivityService::name() const { return "ConnectivityService"; }
void ConnectivityService::init(ServiceContext&) {}
void ConnectivityService::start(ServiceContext&) {}
void ConnectivityService::handle_event(const Event& event, ServiceContext& context) {
    switch (event.type) {
        case EventType::SpotifyConnectionLost:
        case EventType::WifiConnectionLost:
            context.state.set_wifi(false, "spotify lost; opening local library");
            context.state.set_notification({"Spotify connection lost", "Playback stopped. Press OK to continue in Local Library.", true, "open_library"});
            context.state.set_screen(Screen::Popup, FirmwareState::Popup, false);
            context.state.append_console("[Recovery] Spotify/Wi-Fi loss -> local library");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Popup, name(), "spotify loss popup"});
            break;
        case EventType::BluetoothConnectionLost:
            context.state.set_bluetooth(false);
            context.state.set_connection(ConnectionState::BluetoothRecovering, "headphones disconnected");
            context.state.append_console("[Bluetooth] reconnecting");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "bluetooth lost"});
            break;
        default:
            break;
    }
}
void ConnectivityService::update(ServiceContext&, std::chrono::milliseconds) {}
void ConnectivityService::shutdown(ServiceContext&) {}

const char* ClockService::name() const { return "ClockService"; }
void ClockService::init(ServiceContext& context) {
    const auto clock = read_system_clock();
    last_minute_ = clock.time_12h;
    context.state.set_clock(clock);
    context.state.append_console("[Clock] " + clock.time_12h + " " + clock.date_label);
    context.events.publish({EventType::ClockChanged, 0, 0, InputAction::None, context.state.state().current_screen, name(), "clock initialized"});
}
void ClockService::start(ServiceContext&) {}
void ClockService::handle_event(const Event&, ServiceContext&) {}
void ClockService::update(ServiceContext& context, std::chrono::milliseconds) {
    const auto clock = read_system_clock();
    if (clock.time_12h == last_minute_ && clock.date_label == context.state.state().clock.date_label) {
        return;
    }
    last_minute_ = clock.time_12h;
    context.state.set_clock(clock);
    context.events.publish({EventType::ClockChanged, 0, 0, InputAction::None, context.state.state().current_screen, name(), "clock minute changed"});
    context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "clock changed"});
}
void ClockService::shutdown(ServiceContext&) {}

ClockSnapshot ClockService::read_system_clock() const {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    std::ostringstream time;
    std::ostringstream date;
    time << std::put_time(&local, "%I:%M %p");
    date << std::put_time(&local, "%Y-%m-%d");
    return {time.str(), date.str(), "system", true};
}

const char* PowerService::name() const { return "PowerService"; }
void PowerService::init(ServiceContext& context) {
    context.state.apply_power_budget("boot");
}
void PowerService::start(ServiceContext&) {}
void PowerService::handle_event(const Event& event, ServiceContext& context) {
    const AppState& state = context.state.state();
    switch (event.type) {
        case EventType::BatterySaverToggleRequested:
            if (state.current_screen == Screen::Sleep || state.current_screen == Screen::Aod) {
                context.events.publish({EventType::WakeRequested, 0, 0, InputAction::None, Screen::Home, name(), "power button wake"});
                context.state.reset_to_screen(Screen::Home, FirmwareState::Home);
            } else {
                context.state.set_power_mode(
                    state.power.mode == PowerMode::BatterySaver ? PowerMode::Normal : PowerMode::BatterySaver,
                    "power button short press");
            }
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "power state changed"});
            break;
        case EventType::SleepRequested:
            context.state.set_screen(Screen::Sleep, FirmwareState::Sleep, false);
            sleep_elapsed_ = std::chrono::milliseconds(0);
            context.state.append_console("[Power] sleep");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Sleep, name(), "sleep entered"});
            break;
        case EventType::ShutdownRequested:
            context.state.set_screen(Screen::Shutdown, FirmwareState::Shutdown, false);
            context.state.set_running(false);
            context.state.append_console("[Power] shutdown");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Shutdown, name(), "shutdown requested"});
            break;
        case EventType::RebootRequested:
            context.state.set_reboot_requested(true);
            context.state.set_screen(Screen::Shutdown, FirmwareState::Shutdown, false);
            context.state.set_running(false);
            context.state.append_console("[Power] reboot");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, Screen::Shutdown, name(), "reboot requested"});
            break;
        case EventType::QuitRequested:
            context.state.set_running(false);
            break;
        case EventType::LowBatterySimulated:
            context.state.set_battery(10, false);
            context.state.apply_power_budget("simulated low battery");
            context.state.set_connection(ConnectionState::LowPower, "critical battery");
            context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "low battery"});
            break;
        default:
            break;
    }
}
void PowerService::update(ServiceContext& context, std::chrono::milliseconds delta) {
    if (context.state.state().current_screen != Screen::Sleep) return;
    sleep_elapsed_ += delta;
    if (sleep_elapsed_ >= std::chrono::minutes(45) && context.state.state().playback.state != PlaybackState::Playing) {
        context.events.publish({EventType::ShutdownRequested, 0, 0, InputAction::None, Screen::Shutdown, name(), "sleep idle timeout"});
    }
}
void PowerService::shutdown(ServiceContext&) {}

BatteryService::BatteryService(Battery& battery) : battery_(battery) {}
const char* BatteryService::name() const { return "BatteryService"; }
void BatteryService::init(ServiceContext& context) {
    const int percent = battery_.percent();
    const bool charging = battery_.is_charging();
    context.state.set_battery(percent, charging);
    if (charging && context.state.state().playback.state != PlaybackState::Playing) {
        context.state.set_screen(Screen::Charging, FirmwareState::Charging, true);
        context.state.append_console("[Power] charging screen");
    }
    context.state.apply_power_budget("battery init");
}
void BatteryService::start(ServiceContext&) {}
void BatteryService::handle_event(const Event&, ServiceContext&) {}
void BatteryService::update(ServiceContext& context, std::chrono::milliseconds delta) {
    elapsed_ += delta;
    if (elapsed_ < std::chrono::seconds(5)) return;
    elapsed_ = std::chrono::milliseconds(0);
    const int next_percent = battery_.percent();
    const bool charging = battery_.is_charging();
    const AppState& state = context.state.state();
    if (next_percent != state.battery_percent || charging != state.charging) {
        context.state.set_battery(next_percent, charging);
        context.state.apply_power_budget("battery update");
        if (charging && state.playback.state != PlaybackState::Playing && state.current_screen != Screen::Charging) {
            context.state.set_screen(Screen::Charging, FirmwareState::Charging, true);
            context.state.append_console("[Power] charging screen");
        } else if (!charging && state.current_screen == Screen::Charging) {
            context.state.reset_to_screen(Screen::Home, FirmwareState::Home);
            context.state.append_console("[Power] charging finished");
        }
        context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "battery changed"});
    }
}
void BatteryService::shutdown(ServiceContext&) {}

BluetoothService::BluetoothService(Bluetooth& bluetooth) : bluetooth_(bluetooth) {}
const char* BluetoothService::name() const { return "BluetoothService"; }
void BluetoothService::init(ServiceContext& context) {
    context.state.set_bluetooth(bluetooth_.connected());
}
void BluetoothService::start(ServiceContext&) {}
void BluetoothService::handle_event(const Event&, ServiceContext&) {}
void BluetoothService::update(ServiceContext& context, std::chrono::milliseconds delta) {
    elapsed_ += delta;
    if (elapsed_ < std::chrono::seconds(3)) return;
    elapsed_ = std::chrono::milliseconds(0);
    const bool connected = bluetooth_.connected();
    if (connected != context.state.state().bluetooth_connected) {
        context.state.set_bluetooth(connected);
        context.events.publish({EventType::RenderRequested, 0, 0, InputAction::None, context.state.state().current_screen, name(), "bluetooth changed"});
    }
}
void BluetoothService::shutdown(ServiceContext&) {}

RenderService::RenderService(Display& display) : display_(display) {}
const char* RenderService::name() const { return "RenderService"; }
void RenderService::init(ServiceContext& context) {
    display_.render(screen_manager_.build_render_model(context.state.state()));
    context.state.set_dirty(false);
}
void RenderService::start(ServiceContext&) {}
void RenderService::handle_event(const Event& event, ServiceContext& context) {
    if (event.type == EventType::RenderRequested) {
        context.state.mark_dirty();
    }
}
void RenderService::update(ServiceContext& context, std::chrono::milliseconds delta) {
    elapsed_ += delta;
    const AppState& state = context.state.state();
    const int fps = std::max(1, state.power.display_budget_fps);
    const auto frame_interval = std::chrono::milliseconds(1000 / fps);
    const bool animated_charging = state.current_screen == Screen::Charging && state.charging;
    if (animated_charging) {
        context.state.mark_dirty();
    }
    if (!context.state.state().dirty || elapsed_ < frame_interval) return;
    elapsed_ = std::chrono::milliseconds(0);
    display_.render(screen_manager_.build_render_model(context.state.state()));
    context.state.set_dirty(false);
}
void RenderService::shutdown(ServiceContext& context) {
    display_.render(screen_manager_.build_render_model(context.state.state()));
}

}  // namespace shaer
