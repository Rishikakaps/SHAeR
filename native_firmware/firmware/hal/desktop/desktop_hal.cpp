#include "desktop_hal.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <sys/select.h>
#include <unistd.h>

namespace shaer {

namespace {

std::string source_label(PlaybackSource source) {
    switch (source) {
        case PlaybackSource::None: return "Idle";
        case PlaybackSource::Local: return "Local";
        case PlaybackSource::Spotify: return "Spotify";
        case PlaybackSource::Bluetooth: return "Bluetooth";
    }
    return "Unknown";
}

std::string state_label(PlaybackState state) {
    switch (state) {
        case PlaybackState::Stopped: return "Stopped";
        case PlaybackState::Playing: return "Playing";
        case PlaybackState::Paused: return "Paused";
    }
    return "Unknown";
}

}  // namespace

DesktopDisplay::DesktopDisplay() : renderer_(std::cout) {}

void DesktopDisplay::render(const RenderModel& model) {
    renderer_.begin_frame();
    auto line = [this](const std::string& text) { renderer_.draw_text(text); };
    std::ostringstream row;

    line("+------------------------------------------+");
    line("|                  SHAeR                   |");
    line("+------------------------------------------+");
    line(std::string("| State:  ") + to_string(model.firmware_state));
    line(std::string("| Screen: ") + to_string(model.screen));
    line(std::string("| Theme:  ") + model.theme.display_name + " / ThemeAPI v1");
    line(std::string("| World:  ") + model.theme.layout_signature);
    line(std::string("| Motion: ") + model.theme.transition_signature);
    line(std::string("| Screen: ") + model.blueprint.chrome + " / " + model.blueprint.primary_region + " / " + model.blueprint.selector);
    row << "| Anim:   " << model.animation.target_fps << "fps, "
        << model.animation.max_animated_elements << " moving parts, "
        << model.animation.reason;
    line(row.str());
    row.str("");
    row.clear();
    row << "| Move:   " << to_string(model.transition.from) << " -> "
        << to_string(model.transition.to) << " / "
        << model.transition.style << " / "
        << model.transition.duration_ms << "ms"
        << (model.transition.blocks_input ? " / modal" : "");
    line(row.str());
    row.str("");
    row.clear();
    row << "| Battery: " << model.battery_percent << "%   WiFi: "
        << (model.wifi_connected ? "Connected" : "Lost")
        << "   Bluetooth: " << (model.bluetooth_connected ? "Connected" : "Off");
    line(row.str());
    row.str("");
    row.clear();
    row << "| Time: " << model.clock.time_12h << "   Date: " << model.clock.date_label
        << "   Clock: " << model.clock.source;
    line(row.str());
    row.str("");
    row.clear();
    row << "| Spotify: "
        << (model.playback.source == PlaybackSource::Spotify ? state_label(model.playback.state) : "Ready")
        << "   Audio: " << source_label(model.playback.source);
    line(row.str());
    row.str("");
    row.clear();
    line(std::string("| Volume: ") + std::to_string(model.volume));
    line(std::string("| Link: ") + to_string(model.connection_state) + " / " + model.connection_hint);
    row << "| Power: " << to_string(model.power.mode) << " / "
        << model.power.display_budget_fps << "fps / "
        << (model.power.wifi_power_save ? "wifi-save" : "wifi-live");
    line(row.str());
    row.str("");
    row.clear();
    line("+------------------------------------------+");

    if (model.screen == Screen::Popup) {
        line(std::string("| ") + model.blueprint.chrome);
        line(std::string("| ") + model.popup.title);
        line(std::string("| ") + model.popup.body);
        line("| Press X / ok to continue.");
    } else if (model.screen == Screen::Library) {
        line(std::string("| ") + model.blueprint.primary_region);
        if (model.local_library.empty()) {
            line("|   No local tracks found");
            line("|   Put MP3/FLAC/WAV files in the music folder");
        }
        for (size_t i = 0; i < model.local_library.size(); ++i) {
            const auto& track = model.local_library[i];
            line(std::string("| ") + (static_cast<int>(i) == model.selected_index ? "> " : "  ") +
                 track.title + " - " + track.artist);
        }
    } else if (model.screen == Screen::VoiceArchive) {
        line(std::string("| ") + model.blueprint.primary_region);
        for (size_t i = 0; i < model.blueprint.lines.size(); ++i) {
            line(std::string("| ") + (static_cast<int>(i) == model.selected_index ? "> " : "  ") + model.blueprint.lines[i]);
        }
    } else if (model.screen == Screen::NowPlaying) {
        line(std::string("| ") + model.blueprint.primary_region);
        line(std::string("| ") + model.playback.track.title);
        line(std::string("| ") + model.playback.track.artist);
        line(std::string("| Album: ") + model.playback.track.album);
        line(std::string("| Source: ") + source_label(model.playback.source));
        row << "| Queue: " << model.playback.queue_index << "/" << model.playback.queue_size
            << "  " << model.playback.progress_seconds << "s/" << model.playback.duration_seconds << "s";
        line(row.str());
        row.str("");
        row.clear();
    } else if (model.screen == Screen::Settings) {
        line(std::string("| ") + model.blueprint.primary_region);
        if (!model.settings_ui.inside_category) {
            for (size_t i = 0; i < model.settings_ui.categories.size(); ++i) {
                line(std::string("| ") + (static_cast<int>(i) == model.settings_ui.category_index ? "> " : "  ") + model.settings_ui.categories[i].title);
            }
        } else if (!model.settings_ui.categories.empty()) {
            const int category = std::clamp(
                model.settings_ui.category_index,
                0,
                std::max(0, static_cast<int>(model.settings_ui.categories.size()) - 1));
            line(std::string("| [") + model.settings_ui.categories[static_cast<size_t>(category)].title + "]");
            const auto& rows = model.settings_ui.categories[static_cast<size_t>(category)].items;
            for (size_t i = 0; i < rows.size(); ++i) {
                const auto& row = rows[i];
                line(std::string("| ") + (static_cast<int>(i) == model.settings_ui.item_index ? "> " : "  ") +
                     row.label + ": " + row.value + (row.placeholder ? " (placeholder)" : ""));
            }
        }
    } else {
        line(std::string("| ") + model.blueprint.primary_region);
        for (size_t i = 0; i < model.blueprint.lines.size(); ++i) {
            line(std::string("| ") + (static_cast<int>(i) == model.selected_index ? "> " : "  ") + model.blueprint.lines[i]);
        }
        line("| Z/back  X/ok  C/play  A/D scroll");
        line("| Commands: library, settings, theme, spotify_play, spotify_drop");
    }

    line("+------------------------------------------+");
    line("| Console");
    for (const auto& line : model.console) {
        renderer_.draw_text("| " + line);
    }
    line("+------------------------------------------+");
    renderer_.present();
}

void DesktopAudioOutput::play_local(const Track& track) {
    track_ = track;
    track_.source = PlaybackSource::Local;
    source_ = PlaybackSource::Local;
    state_ = PlaybackState::Playing;
}

void DesktopAudioOutput::play_spotify(const Track& track) {
    track_ = track;
    track_.source = PlaybackSource::Spotify;
    source_ = PlaybackSource::Spotify;
    state_ = PlaybackState::Playing;
}

void DesktopAudioOutput::set_volume(int volume) {
    volume_ = std::clamp(volume, 0, 100);
}

void DesktopAudioOutput::stop() {
    state_ = PlaybackState::Stopped;
    source_ = PlaybackSource::None;
}

void DesktopAudioOutput::pause() {
    state_ = state_ == PlaybackState::Playing ? PlaybackState::Paused : PlaybackState::Playing;
}

int DesktopBattery::percent() const {
    return percent_;
}

bool DesktopBattery::is_charging() const {
    return false;
}

void DesktopBattery::set_percent(int percent) {
    percent_ = std::clamp(percent, 0, 100);
}

bool DesktopBluetooth::connected() const {
    return connected_;
}

void DesktopBluetooth::set_connected(bool connected) {
    connected_ = connected;
}

InputAction DesktopInput::next_action() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        return InputAction::Quit;
    }
    return parse(line);
}

InputAction DesktopInput::poll_action() {
    timeval timeout{};
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    const int ready = select(STDIN_FILENO + 1, &read_set, nullptr, nullptr, &timeout);
    if (ready <= 0 || !FD_ISSET(STDIN_FILENO, &read_set)) {
        return InputAction::None;
    }
    return next_action();
}

InputAction DesktopInput::parse(const std::string& line) const {
    if (line == "x" || line == "ok") return InputAction::Confirm;
    if (line == "z" || line == "back") return InputAction::Back;
    if (line == "w" || line == "up") return InputAction::Up;
    if (line == "s" || line == "down") return InputAction::Down;
    if (line == "c" || line == "play") return InputAction::PlayPause;
    if (line == "next") return InputAction::Next;
    if (line == "prev") return InputAction::Previous;
    if (line == "vol+" || line == "+") return InputAction::VolumeUp;
    if (line == "vol-" || line == "-") return InputAction::VolumeDown;
    if (line == "library") return InputAction::OpenLibrary;
    if (line == "marginalia" || line == "notes") return InputAction::OpenMarginalia;
    if (line == "settings") return InputAction::OpenSettings;
    if (line == "theme") return InputAction::CycleTheme;
    if (line == "spotify_play") return InputAction::StartSpotify;
    if (line == "spotify_drop") return InputAction::SimulateSpotifyDisconnect;
    if (line == "bt_drop") return InputAction::SimulateBluetoothDisconnect;
    if (line == "low_battery") return InputAction::SimulateLowBattery;
    if (line == "battery_saver") return InputAction::ToggleBatterySaver;
    if (line == "sleep") return InputAction::EnterSleep;
    if (line == "shutdown") return InputAction::BeginShutdown;
    if (line == "reboot") return InputAction::Reboot;
    if (line == "q" || line == "quit") return InputAction::Quit;
    return InputAction::None;
}

}  // namespace shaer
