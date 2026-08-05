#include "ui_framework.hpp"

#include <cassert>
#include <iostream>
#include <utility>

namespace {

shaer::RenderModel base_model(shaer::Screen screen) {
    shaer::RenderModel model;
    model.screen = screen;
    if (screen == shaer::Screen::Library) {
        model.firmware_state = shaer::FirmwareState::LocalLibrary;
    } else if (screen == shaer::Screen::NowPlaying) {
        model.firmware_state = shaer::FirmwareState::Playback;
    } else if (screen == shaer::Screen::Settings) {
        model.firmware_state = shaer::FirmwareState::Settings;
    } else if (screen == shaer::Screen::Boot) {
        model.firmware_state = shaer::FirmwareState::Booting;
    } else {
        model.firmware_state = shaer::FirmwareState::Home;
    }
    model.battery_percent = 87;
    model.wifi_connected = true;
    model.bluetooth_connected = false;
    model.clock.time_12h = "07:45 PM";
    model.clock.date_label = "2026-06-30";
    model.clock.valid = true;
    model.volume = 55;
    model.power.mode = shaer::PowerMode::Normal;
    model.power.display_budget_fps = 60;
    model.selected_index = 1;
    return model;
}

bool has_command(const shaer::UiFrame& frame, shaer::UiCommandType type, const std::string& text = {}) {
    for (const auto& command : frame.commands) {
        if (command.type != type) continue;
        if (text.empty() || command.text == text) return true;
    }
    return false;
}

bool has_progress_value(const shaer::UiFrame& frame, int value, int max_value) {
    for (const auto& command : frame.commands) {
        if (command.type == shaer::UiCommandType::Progress &&
            command.value == value &&
            command.max_value == max_value) {
            return true;
        }
    }
    return false;
}

std::string frame_signature(const shaer::UiFrame& frame) {
    std::string out;
    for (const auto& command : frame.commands) {
        out += std::to_string(static_cast<int>(command.type)) + ":";
        out += std::to_string(command.rect.x) + ",";
        out += std::to_string(command.rect.y) + ",";
        out += std::to_string(command.rect.w) + ",";
        out += std::to_string(command.rect.h) + ":";
        out += command.text + ":";
        out += std::to_string(command.value) + "/" + std::to_string(command.max_value) + ";";
    }
    return out;
}

shaer::UiFrame charging_frame(std::string theme_id, int tick, int battery) {
    shaer::UiFramework ui;
    auto model = base_model(shaer::Screen::Charging);
    model.firmware_state = shaer::FirmwareState::Charging;
    model.theme.id = std::move(theme_id);
    model.theme.definition.palette.background = {0, 12, 14};
    model.theme.definition.palette.foreground = {232, 236, 232};
    model.theme.definition.palette.accent = {0, 232, 139};
    model.theme.definition.palette.popup = {0, 6, 8};
    model.theme.definition.palette.progress_background = {52, 52, 52};
    model.tick_count = tick;
    model.battery_percent = battery;
    model.charging = true;
    return ui.build_frame(model);
}

shaer::Track track(std::string title, std::string artist = "Artist", std::string album = "Album") {
    shaer::Track out;
    out.title = std::move(title);
    out.artist = std::move(artist);
    out.album = std::move(album);
    out.file_path = "/music/" + out.title + ".mp3";
    out.source = shaer::PlaybackSource::Local;
    return out;
}

void home_has_archive_dark_shell_and_selection() {
    shaer::UiFramework ui;
    const auto frame = ui.build_frame(base_model(shaer::Screen::Home));
    assert(frame.width == 240);
    assert(frame.height == 320);
    assert(has_command(frame, shaer::UiCommandType::Text, "Home"));
    assert(has_command(frame, shaer::UiCommandType::Text, "[1] SPOTIFY CONNECT"));
    bool selected = false;
    for (const auto& command : frame.commands) {
        selected = selected || command.selected;
    }
    assert(selected);
}

void library_scrolls_around_selected_track() {
    shaer::UiFramework ui;
    auto model = base_model(shaer::Screen::Library);
    model.selected_index = 4;
    for (int i = 0; i < 8; ++i) {
        model.local_library.push_back(track("Track " + std::to_string(i)));
    }
    const auto frame = ui.build_frame(model);
    assert(has_command(frame, shaer::UiCommandType::Text, "Track 4"));
    assert(has_command(frame, shaer::UiCommandType::Text, "Track 6"));
}

void now_playing_has_artwork_and_progress() {
    shaer::UiFramework ui;
    auto model = base_model(shaer::Screen::NowPlaying);
    model.playback.state = shaer::PlaybackState::Playing;
    model.playback.track = track("Breadboard Song", "Local Artist", "Album");
    model.playback.progress_seconds = 30;
    model.playback.duration_seconds = 120;
    const auto frame = ui.build_frame(model);
    assert(has_command(frame, shaer::UiCommandType::Text, "NOW PLAYING"));
    assert(has_command(frame, shaer::UiCommandType::Progress));
}

void boot_uses_boot_layout_without_sidebar() {
    shaer::UiFramework ui;
    const auto frame = ui.build_frame(base_model(shaer::Screen::Boot));
    assert(has_command(frame, shaer::UiCommandType::Text, "SHAER"));
    assert(has_command(frame, shaer::UiCommandType::Text, "POWERED BY ADI-VASI"));
    assert(!has_command(frame, shaer::UiCommandType::Icon, "HM"));
}

void charging_screens_are_theme_specific_and_connected() {
    const auto archive_a = charging_frame("archive_dark", 0, 30);
    const auto archive_b = charging_frame("archive_dark", 40, 30);
    assert(!has_command(archive_a, shaer::UiCommandType::Image));
    assert(has_command(archive_a, shaer::UiCommandType::Text, "ARCHIVE WALK"));
    assert(frame_signature(archive_a) != frame_signature(archive_b));

    const auto bombay = charging_frame("bombay_ticket", 30, 100);
    assert(!has_command(bombay, shaer::UiCommandType::Image));
    assert(has_command(bombay, shaer::UiCommandType::Text, "HOGYA"));
    assert(has_progress_value(bombay, 100, 100));

    const auto punk_a = charging_frame("japanese_punk", 0, 68);
    const auto punk_b = charging_frame("japanese_punk", 80, 68);
    assert(!has_command(punk_a, shaer::UiCommandType::Image));
    assert(frame_signature(punk_a) == frame_signature(punk_b));

    const auto xp = charging_frame("windows_xp", 10, 55);
    assert(!has_command(xp, shaer::UiCommandType::Image));
    assert(has_progress_value(xp, 55, 100));

    const auto ghibli_a = charging_frame("ghibli_garden", 0, 44);
    const auto ghibli_b = charging_frame("ghibli_garden", 96, 44);
    assert(!has_command(ghibli_a, shaer::UiCommandType::Image));
    assert(frame_signature(ghibli_a) != frame_signature(ghibli_b));

    const auto raga = charging_frame("indian_raga", 120, 72);
    assert(!has_command(raga, shaer::UiCommandType::Image));
}

void every_theme_renders_every_core_screen() {
    const std::vector<std::string> theme_ids = {
        "archive_dark",
        "bombay_ticket",
        "japanese_punk",
        "windows_xp",
        "ghibli_garden",
        "indian_raga",
    };
    const std::vector<shaer::Screen> screens = {
        shaer::Screen::Boot,
        shaer::Screen::Home,
        shaer::Screen::Library,
        shaer::Screen::NowPlaying,
        shaer::Screen::VoiceArchive,
        shaer::Screen::BluetoothConnect,
        shaer::Screen::Settings,
        shaer::Screen::About,
        shaer::Screen::Popup,
        shaer::Screen::Charging,
    };
    shaer::UiFramework ui;
    for (const auto& theme_id : theme_ids) {
        std::string signatures;
        for (const auto screen : screens) {
            auto model = base_model(screen);
            model.theme.id = theme_id;
            model.theme.definition.palette.background = {1, 2, 3};
            model.theme.definition.palette.foreground = {230, 230, 220};
            model.theme.definition.palette.accent = {0, 220, 130};
            model.theme.definition.palette.selection = {0, 220, 130};
            model.theme.definition.palette.disabled = {120, 120, 120};
            model.theme.definition.palette.popup = {16, 16, 16};
            model.theme.definition.palette.border = {80, 80, 80};
            model.theme.definition.palette.status_bar = {22, 22, 22};
            model.theme.definition.palette.footer = {22, 22, 22};
            model.theme.definition.palette.progress_background = {40, 40, 40};
            model.theme.definition.palette.progress_foreground = {230, 230, 220};
            model.popup = {"SPOTIFY FOUND", "MAIN PHONE READY", true, "return_previous"};
            model.tick_count = 64;
            const auto frame = ui.build_frame(model);
            assert(frame.width == 240);
            assert(frame.height == 320);
            assert(!frame.commands.empty());
            assert(!has_command(frame, shaer::UiCommandType::Image));
            signatures += frame_signature(frame);
        }
        assert(!signatures.empty());
    }
}

}  // namespace

int main() {
    home_has_archive_dark_shell_and_selection();
    library_scrolls_around_selected_track();
    now_playing_has_artwork_and_progress();
    boot_uses_boot_layout_without_sidebar();
    charging_screens_are_theme_specific_and_connected();
    every_theme_renders_every_core_screen();
    std::cout << "ui_framework_tests passed\n";
    return 0;
}
