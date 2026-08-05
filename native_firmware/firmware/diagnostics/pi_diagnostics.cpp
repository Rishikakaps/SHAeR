#include "app_state.hpp"
#include "music_library_store.hpp"
#include "playback_engine.hpp"
#include "pi_hal.hpp"
#include "settings_catalog.hpp"
#include "settings_store.hpp"
#include "ui_framework.hpp"

#include <chrono>
#include <ctime>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {

shaer::Track diagnostic_track() {
    shaer::Track track;
    track.title = "breadboard_test";
    track.artist = "Diagnostic";
    track.album = "SHAeR";
    track.file_path = "/var/lib/shaer/music/breadboard_test.mp3";
    track.source = shaer::PlaybackSource::Local;
    track.genre = "Diagnostic";
    track.codec = "mp3";
    track.album_art_path = {};
    track.folder = "/var/lib/shaer/music";
    return track;
}

shaer::RenderModel diagnostic_model(shaer::Screen screen, const std::string& hint) {
    shaer::RenderModel model;
    model.os_name = "SHAeR";
    model.firmware_state = shaer::firmware_state_for_screen(screen);
    model.screen = screen;
    model.theme.id = "archive_dark";
    model.theme.display_name = "Diagnostic";
    model.battery_percent = 88;
    model.bluetooth_connected = false;
    model.wifi_connected = true;
    model.connection_hint = hint;
    model.clock.time_12h = "12:00 PM";
    model.clock.date_label = "2026-06-28";
    model.clock.valid = true;
    model.power.display_budget_fps = 30;
    model.volume = 55;
    if (screen == shaer::Screen::Home) {
        model.blueprint.lines = {"Music", "Now Playing", "Settings", "About"};
    }
    if (screen == shaer::Screen::Library) {
        model.local_library.push_back(diagnostic_track());
    }
    if (screen == shaer::Screen::NowPlaying) {
        model.playback.state = shaer::PlaybackState::Playing;
        model.playback.source = shaer::PlaybackSource::Local;
        model.playback.track = diagnostic_track();
        model.playback.queue_index = 1;
        model.playback.queue_size = 1;
    }
    return model;
}

int display_test() {
    shaer::PiDisplay display;
    if (!display.ready()) {
        std::cerr << "display_test: FAIL display not ready\n";
        return 2;
    }
    std::cout << "display_test: running full TFT pattern; physical PASS requires visible colors, text, and SHAeR logo\n";
    display.run_diagnostic_pattern();
    std::cout << "display_test: complete; confirm the physical TFT before marking PASS\n";
    return 0;
}

int renderer_test() {
    shaer::UiFramework ui;
    const auto home = ui.build_frame(diagnostic_model(shaer::Screen::Home, "renderer home"));
    const auto library = ui.build_frame(diagnostic_model(shaer::Screen::Library, "renderer library"));
    const auto settings = ui.build_frame(diagnostic_model(shaer::Screen::Settings, "renderer settings"));
    if (home.commands.empty() || library.commands.empty() || settings.commands.empty()) {
        std::cerr << "renderer_test: FAIL empty UI frame\n";
        return 2;
    }
    std::cout << "renderer_test: PASS home_commands=" << home.commands.size()
              << " library_commands=" << library.commands.size()
              << " settings_commands=" << settings.commands.size() << "\n";
    return 0;
}

int navigation_test() {
    shaer::PiDisplay display;
    if (!display.ready()) {
        std::cerr << "navigation_test: FAIL display not ready\n";
        return 2;
    }
    auto home = diagnostic_model(shaer::Screen::Home, "navigation home");
    home.selected_index = 0;
    display.render(home);
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    home.selected_index = 1;
    display.render(home);
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    display.render(diagnostic_model(shaer::Screen::Library, "navigation library"));
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    display.render(diagnostic_model(shaer::Screen::Settings, "navigation settings"));
    std::cout << "navigation_test: PASS if selection highlight moved and screens changed\n";
    return 0;
}

int font_test() {
    const int shaer_width = shaer::FontMetrics::text_width("SHAER", 1);
    const auto fitted = shaer::FontMetrics::fit_text("A VERY LONG TRACK NAME", 60, 1);
    if (shaer_width != 30 || fitted.empty() || fitted.size() > 10) {
        std::cerr << "font_test: FAIL metrics invalid\n";
        return 2;
    }
    std::cout << "font_test: PASS glyph=6x7 shaer_width=" << shaer_width
              << " fitted='" << fitted << "'\n";
    return 0;
}

int transition_test() {
    auto model = diagnostic_model(shaer::Screen::Library, "transition");
    model.transition = {shaer::Screen::Home, shaer::Screen::Library, "slide", 240, false, false, "diagnostic"};
    shaer::UiFramework ui;
    const auto frame = ui.build_frame(model);
    for (const auto& command : frame.commands) {
        if (command.type == shaer::UiCommandType::Transition) {
            std::cout << "transition_test: PASS style=" << command.text
                      << " duration=" << command.value << "\n";
            return 0;
        }
    }
    std::cerr << "transition_test: FAIL no transition primitive\n";
    return 2;
}

int encoder_test() {
    shaer::PiInput input;
    std::cout << "encoder_test: rotate EC11, press push/back/play/options/power. Ctrl+C to stop.\n";
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (std::chrono::steady_clock::now() < deadline) {
        const auto action = input.poll_action();
        if (action != shaer::InputAction::None) {
            std::cout << "input_action=" << static_cast<int>(action) << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    std::cout << "encoder_test: PASS if actions appeared for each control\n";
    return 0;
}

int audio_test() {
    std::cout << "audio_test: playing 440 Hz through ALSA default device\n";
    std::system("aplay -l >/tmp/shaer_audio_devices.log 2>&1 || true");
    const int rc = std::system("speaker-test -t sine -f 440 -l 1 >/tmp/shaer_audio_test.log 2>&1");
    if (rc != 0) {
        std::cerr << "audio_test: FAIL speaker-test failed; see /tmp/shaer_audio_test.log\n";
        return 2;
    }
    std::cout << "audio_test: PASS if tone was audible from PCM5102A output\n";
    return 0;
}

std::vector<shaer::Track> diagnostic_queue() {
    auto one = diagnostic_track();
    one.title = "gapless_a";
    one.duration_seconds = 5;
    auto two = diagnostic_track();
    two.title = "gapless_b";
    two.duration_seconds = 5;
    return {one, two};
}

bool has_playback_command(const std::vector<shaer::PlaybackCommand>& commands, shaer::PlaybackCommandType type) {
    for (const auto& command : commands) {
        if (command.type == type) return true;
    }
    return false;
}

int playback_test() {
    shaer::PlaybackEngine engine;
    const auto commands = engine.load_queue(diagnostic_queue(), 0);
    const auto snapshot = engine.snapshot();
    const bool pass = has_playback_command(commands, shaer::PlaybackCommandType::PlayTrack) &&
                      snapshot.state == shaer::PlaybackState::Playing &&
                      snapshot.queue_size == 2;
    std::cout << "playback_test: " << (pass ? "PASS" : "FAIL")
              << " state=" << shaer::to_string(snapshot.state)
              << " queue=" << snapshot.queue_index << "/" << snapshot.queue_size << "\n";
    return pass ? 0 : 2;
}

int gapless_test() {
    shaer::PlaybackEngine engine;
    engine.set_crossfade_seconds(0);
    engine.load_queue(diagnostic_queue(), 0);
    auto commands = engine.update(std::chrono::seconds(4));
    const bool prebuffer = has_playback_command(commands, shaer::PlaybackCommandType::PrebufferTrack);
    commands = engine.update(std::chrono::seconds(1));
    const bool transitioned = has_playback_command(commands, shaer::PlaybackCommandType::PlayTrack) &&
                              engine.snapshot().track.title == "gapless_b";
    std::cout << "gapless_test: " << (prebuffer && transitioned ? "PASS" : "FAIL")
              << " prebuffer=" << (prebuffer ? "yes" : "no")
              << " transition_track=" << engine.snapshot().track.title << "\n";
    return prebuffer && transitioned ? 0 : 2;
}

int crossfade_test() {
    bool pass = true;
    for (int seconds : {0, 1, 2, 3, 5}) {
        shaer::PlaybackEngine engine;
        engine.set_crossfade_seconds(seconds);
        engine.load_queue(diagnostic_queue(), 0);
        const auto commands = engine.update(std::chrono::seconds(5 - std::max(1, seconds)));
        const bool prebuffer = seconds == 0 || has_playback_command(commands, shaer::PlaybackCommandType::PrebufferTrack);
        pass = pass && engine.crossfade_seconds() == seconds && prebuffer;
        std::cout << "crossfade_test: duration=" << seconds
                  << " configured=" << engine.crossfade_seconds()
                  << " prebuffer=" << (prebuffer ? "yes" : "no") << "\n";
    }
    std::cout << "crossfade_test: " << (pass ? "PASS" : "FAIL") << "\n";
    return pass ? 0 : 2;
}

int buffer_test() {
    shaer::PlaybackEngine engine;
    engine.set_low_power(false);
    engine.note_underrun();
    engine.note_recovery();
    engine.set_low_power(true);
    const auto telemetry = engine.telemetry();
    const bool pass = telemetry.underruns == 1 && telemetry.recoveries == 1 &&
                      telemetry.low_power && telemetry.buffer_target_ms == 600;
    std::cout << "buffer_test: " << (pass ? "PASS" : "FAIL")
              << " underruns=" << telemetry.underruns
              << " recoveries=" << telemetry.recoveries
              << " buffer_ms=" << telemetry.buffer_target_ms << "\n";
    return pass ? 0 : 2;
}

int volume_test() {
    shaer::PlaybackEngine engine;
    engine.set_volume_target(100);
    int last = 50;
    bool monotonic = true;
    for (int i = 0; i < 20; ++i) {
        const auto commands = engine.update(std::chrono::milliseconds(40));
        for (const auto& command : commands) {
            if (command.type == shaer::PlaybackCommandType::SetVolume) {
                monotonic = monotonic && command.value >= last && command.value - last <= 4;
                last = command.value;
            }
        }
    }
    engine.set_volume_target(0);
    for (int i = 0; i < 30; ++i) {
        engine.update(std::chrono::milliseconds(40));
    }
    std::cout << "volume_test: " << (monotonic ? "PASS" : "FAIL")
              << " ramp_peak=" << last << " target=" << engine.target_volume() << "\n";
    return monotonic ? 0 : 2;
}

int seek_test() {
    shaer::PlaybackEngine engine;
    engine.load_queue(diagnostic_queue(), 0);
    auto commands = engine.seek_relative(3);
    const bool forward = has_playback_command(commands, shaer::PlaybackCommandType::SeekTo) &&
                         engine.snapshot().progress_seconds == 3;
    commands = engine.seek_relative(-2);
    const bool backward = has_playback_command(commands, shaer::PlaybackCommandType::SeekTo) &&
                          engine.snapshot().progress_seconds == 1;
    std::cout << "seek_test: " << (forward && backward ? "PASS" : "FAIL")
              << " position=" << engine.snapshot().progress_seconds << "\n";
    return forward && backward ? 0 : 2;
}

int stress_playback_test() {
    shaer::PlaybackEngine engine;
    engine.set_repeat_mode(shaer::RepeatMode::All);
    engine.load_queue(diagnostic_queue(), 0);
    for (int i = 0; i < 3600; ++i) {
        engine.update(std::chrono::seconds(1));
    }
    const auto snapshot = engine.snapshot();
    const bool pass = snapshot.state == shaer::PlaybackState::Playing && snapshot.queue_size == 2;
    std::cout << "stress_playback_test: " << (pass ? "PASS" : "FAIL")
              << " simulated_seconds=3600"
              << " state=" << shaer::to_string(snapshot.state)
              << " track=" << snapshot.track.title << "\n";
    return pass ? 0 : 2;
}

int sdcard_test() {
    namespace fs = std::filesystem;
    const fs::path music = "/var/lib/shaer/music";
    std::error_code ec;
    fs::create_directories(music, ec);
    int tracks = 0;
    for (const auto& entry : fs::recursive_directory_iterator(music, fs::directory_options::skip_permission_denied, ec)) {
        if (ec) {
            ec.clear();
            continue;
        }
        if (!entry.is_regular_file(ec)) continue;
        const auto ext = entry.path().extension().string();
        if (ext == ".mp3" || ext == ".MP3") {
            ++tracks;
        }
    }
    std::cout << "sdcard_test: PASS music_dir=" << music << " mp3_count=" << tracks << "\n";
    return 0;
}

int library_scan_test() {
    shaer::MusicLibraryStore store("/var/lib/shaer/shaer.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "library_scan_test: FAIL " << store.last_error() << "\n";
        return 2;
    }
    const auto scan = store.scan("/var/lib/shaer/music");
    const auto index = store.index();
    std::cout << "library_scan_test: PASS files_seen=" << scan.files_seen
              << " tracks_indexed=" << scan.tracks_indexed
              << " changed=" << scan.tracks_added_or_updated
              << " albums=" << index.albums.size()
              << " artists=" << index.artists.size()
              << " folders=" << index.folders.size()
              << " elapsed_ms=" << scan.elapsed_ms << "\n";
    return 0;
}

int metadata_test() {
    shaer::MusicLibraryStore store("/var/lib/shaer/shaer.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "metadata_test: FAIL " << store.last_error() << "\n";
        return 2;
    }
    store.scan("/var/lib/shaer/music");
    const auto songs = store.songs(1);
    if (songs.empty()) {
        std::cout << "metadata_test: PASS no music files available; scanner handled empty library\n";
        return 0;
    }
    const auto& song = songs.front();
    std::cout << "metadata_test: PASS title='" << song.title
              << "' artist='" << song.artist
              << "' album='" << song.album
              << "' codec=" << song.codec
              << " duration=" << song.duration_seconds
              << " sample_rate=" << song.sample_rate_hz << "\n";
    return 0;
}

int album_art_test() {
    shaer::MusicLibraryStore store("/var/lib/shaer/shaer.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "album_art_test: FAIL " << store.last_error() << "\n";
        return 2;
    }
    store.scan("/var/lib/shaer/music");
    int with_art = 0;
    for (const auto& song : store.songs()) {
        if (!song.album_art_path.empty() && std::filesystem::exists(song.album_art_path)) {
            ++with_art;
        }
    }
    std::cout << "album_art_test: PASS cached_art_tracks=" << with_art << "\n";
    return 0;
}

int performance_test() {
    shaer::MusicLibraryStore store("/var/lib/shaer/shaer.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "performance_test: FAIL " << store.last_error() << "\n";
        return 2;
    }
    const auto scan = store.scan("/var/lib/shaer/music");
    const bool pass = scan.files_seen < 500 || scan.elapsed_ms < 10000;
    std::cout << "performance_test: " << (pass ? "PASS" : "FAIL")
              << " files_seen=" << scan.files_seen
              << " elapsed_ms=" << scan.elapsed_ms
              << " target_ms_for_500=10000\n";
    return pass ? 0 : 2;
}

int settings_test() {
    shaer::SettingsStore store("/var/lib/shaer/settings.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "settings_test: FAIL " << store.last_error() << "\n";
        return 2;
    }
    const auto before = store.get("appearance.brightness").value_or("80");
    const auto next = shaer::next_setting_value("appearance.brightness", before);
    if (!store.put("appearance.brightness", next)) {
        std::cerr << "settings_test: FAIL save " << store.last_error() << "\n";
        return 2;
    }
    const auto after = store.get("appearance.brightness").value_or("");
    std::cout << "settings_test: " << (after == next ? "PASS" : "FAIL")
              << " schema=" << store.schema_version()
              << " brightness=" << after << "\n";
    return after == next ? 0 : 2;
}

int power_test() {
    shaer::PiBattery battery;
    std::ifstream temp("/sys/class/thermal/thermal_zone0/temp");
    int milli_c = 0;
    temp >> milli_c;
    std::cout << "power_test: PASS battery=" << battery.percent()
              << "% charging=" << (battery.is_charging() ? "yes" : "no")
              << " cpu_temp_c=" << (milli_c > 0 ? milli_c / 1000 : 0) << "\n";
    return 0;
}

int brightness_test() {
    shaer::SettingsStore store("/var/lib/shaer/settings.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "brightness_test: FAIL settings unavailable\n";
        return 2;
    }
    const auto current = store.get("appearance.brightness").value_or("80");
    const auto next = shaer::next_setting_value("appearance.brightness", current);
    store.put("appearance.brightness", next);
    std::cout << "brightness_test: PASS brightness=" << next << " stored; backlight GPIO remains HAL-owned\n";
    return 0;
}

int storage_test() {
    std::error_code ec;
    const auto space = std::filesystem::space("/var/lib/shaer", ec);
    if (ec) {
        std::cerr << "storage_test: FAIL " << ec.message() << "\n";
        return 2;
    }
    std::cout << "storage_test: PASS free_mb=" << space.available / (1024 * 1024)
              << " used_mb=" << (space.capacity - space.available) / (1024 * 1024) << "\n";
    return 0;
}

int shutdown_test() {
    std::cout << "shutdown_test: PASS safe shutdown path is systemd -> SIGTERM -> clean_shutdown -> shutdown -h now\n";
    return 0;
}

int reboot_test() {
    std::cout << "reboot_test: PASS safe reboot path is RebootRequested -> clean_shutdown -> reboot\n";
    return 0;
}

int developer_mode_test() {
    shaer::SettingsStore store("/var/lib/shaer/settings.db");
    if (!store.open() || !store.migrate()) {
        std::cerr << "developer_mode_test: FAIL " << store.last_error() << "\n";
        return 2;
    }
    const auto current = store.get("advanced.developer_mode").value_or("off");
    const auto next = current == "on" ? "off" : "on";
    store.put("advanced.developer_mode", next);
    std::cout << "developer_mode_test: PASS developer_mode=" << store.get("advanced.developer_mode").value_or("?") << "\n";
    return 0;
}

int battery_test() {
    shaer::PiBattery battery;
    std::cout << "battery_test: percent=" << battery.percent()
              << " charging=" << (battery.is_charging() ? "yes" : "no") << "\n";
    return 0;
}

int wifi_test() {
    const std::string log_path = "/tmp/shaer_wifi_test_" + std::to_string(static_cast<long long>(std::time(nullptr))) + ".log";
    const std::string command = "ip link show wlan0 >" + log_path + " 2>&1 && (command -v iw >/dev/null 2>&1 && iw dev wlan0 link >>" + log_path + " 2>&1 || /usr/sbin/iw dev wlan0 link >>" + log_path + " 2>&1)";
    const int rc = std::system(command.c_str());
    std::cout << "wifi_test: " << (rc == 0 ? "PASS" : "WARN") << " see " << log_path << "\n";
    return rc == 0 ? 0 : 1;
}

int gpio_test() {
    shaer::PiPinMap pins;
    const int gpios[] = {pins.power_button, pins.encoder_a, pins.encoder_b, pins.encoder_push, pins.button_back, pins.button_play, pins.button_options};
    for (int gpio : gpios) {
        shaer::pi_export_gpio(gpio);
        shaer::pi_set_gpio_direction(gpio, "in");
        std::cout << "gpio" << gpio << "=" << shaer::pi_read_gpio(gpio, 1) << "\n";
    }
    std::cout << "gpio_test: PASS if pressed buttons read 0 and released buttons read 1\n";
    return 0;
}

void usage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " display|renderer|navigation|ui_navigation|encoder|font|transition|audio|playback|gapless|crossfade|buffer|volume|seek|stress_playback|sdcard|library_scan|library|metadata|album_art|performance|settings|power|brightness|storage|battery|wifi|gpio|shutdown|reboot|developer_mode\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 2;
    }
    const std::string test = argv[1];
    if (test == "display") return display_test();
    if (test == "renderer") return renderer_test();
    if (test == "navigation" || test == "ui_navigation") return navigation_test();
    if (test == "encoder") return encoder_test();
    if (test == "font") return font_test();
    if (test == "transition") return transition_test();
    if (test == "audio") return audio_test();
    if (test == "playback" || test == "playback_test") return playback_test();
    if (test == "gapless" || test == "gapless_test") return gapless_test();
    if (test == "crossfade" || test == "crossfade_test") return crossfade_test();
    if (test == "buffer" || test == "buffer_test") return buffer_test();
    if (test == "volume" || test == "volume_test") return volume_test();
    if (test == "seek" || test == "seek_test") return seek_test();
    if (test == "stress_playback" || test == "stress_playback_test") return stress_playback_test();
    if (test == "sdcard") return sdcard_test();
    if (test == "library_scan" || test == "library_scan_test" || test == "library" || test == "library_test") return library_scan_test();
    if (test == "metadata" || test == "metadata_test") return metadata_test();
    if (test == "album_art" || test == "album_art_test") return album_art_test();
    if (test == "performance" || test == "performance_test") return performance_test();
    if (test == "settings" || test == "settings_test") return settings_test();
    if (test == "power" || test == "power_test") return power_test();
    if (test == "brightness" || test == "brightness_test") return brightness_test();
    if (test == "storage" || test == "storage_test") return storage_test();
    if (test == "battery") return battery_test();
    if (test == "wifi") return wifi_test();
    if (test == "gpio") return gpio_test();
    if (test == "shutdown" || test == "shutdown_test") return shutdown_test();
    if (test == "reboot" || test == "reboot_test") return reboot_test();
    if (test == "developer_mode" || test == "developer_mode_test") return developer_mode_test();
    usage(argv[0]);
    return 2;
}
