#include "animation_manager.hpp"
#include "event_bus.hpp"
#include "firmware_runtime.hpp"
#include "main_scheduler.hpp"
#include "resource_manager.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace {

class TestInput final : public shaer::Input {
public:
    shaer::InputAction next_action() override { return poll_action(); }
    shaer::InputAction poll_action() override {
        if (next == shaer::InputAction::None) return shaer::InputAction::None;
        const auto action = next;
        next = shaer::InputAction::None;
        return action;
    }
    shaer::InputAction next = shaer::InputAction::None;
};

class TestDisplay final : public shaer::Display {
public:
    void render(const shaer::RenderModel& model) override {
        ++frames;
        last_screen = model.screen;
        last_state = model.firmware_state;
    }
    int frames = 0;
    shaer::Screen last_screen = shaer::Screen::Boot;
    shaer::FirmwareState last_state = shaer::FirmwareState::Booting;
};

class TestAudio final : public shaer::AudioOutput {
public:
    void play_local(const shaer::Track& track) override {
        last_track = track;
        state = shaer::PlaybackState::Playing;
        source = shaer::PlaybackSource::Local;
        ++play_local_calls;
    }
    void play_spotify(const shaer::Track& track) override {
        last_track = track;
        state = shaer::PlaybackState::Playing;
        source = shaer::PlaybackSource::Spotify;
        ++play_spotify_calls;
    }
    void set_volume(int volume) override {
        last_volume = volume;
        ++set_volume_calls;
    }
    void stop() override {
        state = shaer::PlaybackState::Stopped;
        source = shaer::PlaybackSource::None;
        ++stop_calls;
    }
    void pause() override {
        state = state == shaer::PlaybackState::Playing ? shaer::PlaybackState::Paused : shaer::PlaybackState::Playing;
        ++pause_calls;
    }
    shaer::Track last_track;
    shaer::PlaybackState state = shaer::PlaybackState::Stopped;
    shaer::PlaybackSource source = shaer::PlaybackSource::None;
    int play_local_calls = 0;
    int play_spotify_calls = 0;
    int stop_calls = 0;
    int pause_calls = 0;
    int set_volume_calls = 0;
    int last_volume = 0;
};

class TestBattery final : public shaer::Battery {
public:
    int percent() const override { return percent_; }
    bool is_charging() const override { return charging_; }
    int percent_ = 88;
    bool charging_ = false;
};

class TestBluetooth final : public shaer::Bluetooth {
public:
    bool connected() const override { return connected_; }
    bool connected_ = true;
};

}  // namespace

void event_bus_preserves_order() {
    shaer::EventBus bus;
    bus.publish({shaer::EventType::NavigateDown});
    bus.publish({shaer::EventType::ButtonOkPressed});
    const auto events = bus.drain();
    assert(events.size() == 2);
    assert(events[0].sequence == 1);
    assert(events[1].sequence == 2);
    assert(events[0].type == shaer::EventType::NavigateDown);
    assert(events[1].type == shaer::EventType::ButtonOkPressed);
    assert(bus.empty());
}

void animation_manager_reduces_power_cost() {
    shaer::AnimationManager manager;
    manager.register_animation({"background_drift", 1000, "ease", true, 5, 30, 1, false});
    manager.register_animation({"cursor", 300, "linear", true, 1, 30, 10, true});
    const auto normal = manager.active_for(shaer::PowerMode::Normal);
    const auto saver = manager.active_for(shaer::PowerMode::BatterySaver);
    assert(normal.size() == 2);
    assert(saver.size() == 1);
    assert(saver.front().id == "cursor");
    assert(saver.front().fps <= 12);
}

void resource_manager_caches_assets() {
    const std::string dir = "/tmp/shaer_resources_" + std::to_string(getpid());
    std::filesystem::create_directories(dir + "/themes/archive/fonts");
    {
        std::ofstream file(dir + "/themes/archive/fonts/main.font");
        file << "font";
    }
    shaer::ResourceManager resources({dir});
    const auto first = resources.load("archive.main_font", "themes/archive/fonts/main.font");
    const auto second = resources.load("archive.main_font", "themes/archive/fonts/main.font");
    assert(first.has_value());
    assert(second.has_value());
    assert(!first->cached);
    assert(second->cached);
    assert(resources.cache_size() == 1);
    std::filesystem::remove_all(dir);
}

void runtime_routes_input_through_events_to_state() {
    const std::string music_dir = "/tmp/shaer_core_runtime_music_" + std::to_string(getpid());
    std::filesystem::create_directories(music_dir + "/Album");
    {
        std::ofstream file(music_dir + "/Album/Track One.mp3");
        file << "not audio, but a real local file for scanner routing";
    }

    TestDisplay display;
    TestAudio audio;
    TestInput input;
    TestBattery battery;
    TestBluetooth bluetooth;
    shaer::StructuredLogger logger("/tmp/shaer_core_runtime_logs_" + std::to_string(getpid()));
    logger.open();
    shaer::FirmwareRuntime runtime({display, audio, input, battery, bluetooth}, &logger);

    shaer::BootReport report;
    report.settings.active_theme = "archive_dark";
    report.settings.music_directory = music_dir;
    report.settings.volume = 44;
    runtime.apply_boot_report(report);
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Home);
    assert(runtime.state().local_library.size() == 1);
    assert(runtime.state().clock.valid);
    assert(runtime.state().clock.source == "system");
    assert(runtime.state().clock.time_12h.size() == 8);

    input.next = shaer::InputAction::Confirm;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Library);
    assert(runtime.state().firmware_state == shaer::FirmwareState::LocalLibrary);

    input.next = shaer::InputAction::Confirm;
    runtime.run_for_ticks(3);
    assert(runtime.state().current_screen == shaer::Screen::NowPlaying);
    assert(runtime.state().playback.state == shaer::PlaybackState::Playing);
    assert(runtime.state().playback.source == shaer::PlaybackSource::Local);
    assert(audio.play_local_calls == 1);
    assert(audio.last_volume == 44);

    input.next = shaer::InputAction::PlayPause;
    runtime.run_for_ticks(2);
    assert(runtime.state().playback.state == shaer::PlaybackState::Paused);

    input.next = shaer::InputAction::SimulateSpotifyDisconnect;
    runtime.run_for_ticks(3);
    assert(runtime.state().current_screen == shaer::Screen::Popup);
    assert(runtime.state().playback.state == shaer::PlaybackState::Stopped);
    assert(audio.stop_calls >= 1);

    input.next = shaer::InputAction::Confirm;
    runtime.run_for_ticks(3);
    assert(runtime.state().current_screen == shaer::Screen::Library);

    input.next = shaer::InputAction::EnterSleep;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Sleep);
    assert(runtime.state().firmware_state == shaer::FirmwareState::Sleep);

    input.next = shaer::InputAction::ToggleBatterySaver;
    runtime.run_for_ticks(2);
    assert(runtime.state().current_screen == shaer::Screen::Home);
    assert(runtime.state().firmware_state == shaer::FirmwareState::Home);

    input.next = shaer::InputAction::CycleTheme;
    runtime.run_for_ticks(2);
    assert(runtime.state().active_theme == "bombay_ticket");

    input.next = shaer::InputAction::SimulateLowBattery;
    runtime.run_for_ticks(2);
    assert(runtime.state().power.mode == shaer::PowerMode::Critical);

    input.next = shaer::InputAction::VolumeUp;
    runtime.run_for_ticks(2);
    assert(runtime.state().settings.volume == 49);
    assert(audio.last_volume >= 44);
    assert(audio.last_volume <= 49);

    input.next = shaer::InputAction::BeginShutdown;
    runtime.run_for_ticks(2);
    assert(!runtime.state().running);
    assert(runtime.state().firmware_state == shaer::FirmwareState::Shutdown);
    std::filesystem::remove_all(music_dir);
}

int main() {
    event_bus_preserves_order();
    animation_manager_reduces_power_cost();
    resource_manager_caches_assets();
    runtime_routes_input_through_events_to_state();
    return 0;
}
