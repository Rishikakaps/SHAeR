#include "hal.hpp"
#include "shaer_app.hpp"

#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

class FakeDisplay final : public shaer::Display {
public:
    void render(const shaer::RenderModel& model) override {
        frames.push_back(model);
    }
    std::vector<shaer::RenderModel> frames;
};

class FakeAudio final : public shaer::AudioOutput {
public:
    void play_local(const shaer::Track& track) override {
        track_ = track;
        source_ = shaer::PlaybackSource::Local;
        state_ = shaer::PlaybackState::Playing;
    }
    void play_spotify(const shaer::Track& track) override {
        track_ = track;
        source_ = shaer::PlaybackSource::Spotify;
        state_ = shaer::PlaybackState::Playing;
    }
    void set_volume(int) override {}
    void stop() override {
        source_ = shaer::PlaybackSource::None;
        state_ = shaer::PlaybackState::Stopped;
    }
    void pause() override {
        state_ = state_ == shaer::PlaybackState::Playing
            ? shaer::PlaybackState::Paused
            : shaer::PlaybackState::Playing;
    }
private:
    shaer::PlaybackState state_ = shaer::PlaybackState::Stopped;
    shaer::PlaybackSource source_ = shaer::PlaybackSource::None;
    shaer::Track track_;
};

class FakeBattery final : public shaer::Battery {
public:
    int percent() const override { return percent_; }
    bool is_charging() const override { return false; }
    int percent_ = 87;
};

class FakeBluetooth final : public shaer::Bluetooth {
public:
    bool connected() const override { return connected_; }
    bool connected_ = true;
};

struct Fixture {
    FakeDisplay display;
    FakeAudio audio;
    FakeBattery battery;
    FakeBluetooth bluetooth;

    shaer::ShaerApp make_app() {
        return shaer::ShaerApp({&display, &audio, &battery, &bluetooth});
    }
};

void boot_opens_home() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    assert(app.firmware_state() == shaer::FirmwareState::Home);
    assert(app.screen() == shaer::Screen::Home);
    assert(app.navigation_depth() == 1);
    assert(!fixture.display.frames.empty());
}

void back_uses_bounded_clean_stack() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenLibrary);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::NowPlaying);
    assert(app.navigation_depth() == 3);

    app.handle(shaer::InputAction::Back);
    assert(app.screen() == shaer::Screen::Library);
    app.handle(shaer::InputAction::Back);
    assert(app.screen() == shaer::Screen::Home);
    app.handle(shaer::InputAction::Back);
    assert(app.screen() == shaer::Screen::Home);
    assert(app.navigation_depth() == 1);
}

void home_selection_opens_expected_screens() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();

    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::Library);

    app.handle(shaer::InputAction::Back);
    app.handle(shaer::InputAction::Down);
    assert(app.selected_index() == 1);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::BluetoothConnect);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::Popup);
    assert(app.popup().title == "SPOTIFY FOUND");
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::BluetoothConnect);

    app.handle(shaer::InputAction::Back);
    app.handle(shaer::InputAction::Down);
    assert(app.selected_index() == 2);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::VoiceArchive);

    app.handle(shaer::InputAction::Back);
    app.handle(shaer::InputAction::Down);
    assert(app.selected_index() == 3);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::Settings);
}

void selection_is_clamped_per_screen() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    for (int i = 0; i < 20; ++i) {
        app.handle(shaer::InputAction::Down);
    }
    assert(app.selected_index() == 3);
    app.handle(shaer::InputAction::OpenLibrary);
    assert(app.selected_index() == 0);
    for (int i = 0; i < 20; ++i) {
        app.handle(shaer::InputAction::Down);
    }
    assert(app.selected_index() == 2);
}

void settings_selection_is_clamped_and_updates_audio_power() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenSettings);
    for (int i = 0; i < 20; ++i) {
        app.handle(shaer::InputAction::Down);
    }
    assert(app.selected_index() == 4);

    app.handle(shaer::InputAction::Confirm);
    assert(app.power_profile().mode == shaer::PowerMode::BatterySaver);
    assert(app.animation_policy().reduce_motion);
    assert(app.animation_policy().target_fps <= 12);

    app.handle(shaer::InputAction::Up);
    app.handle(shaer::InputAction::Confirm);
    assert(app.audio_settings().quality_mode == shaer::QualityMode::ArchiveQuality);
    assert(app.power_profile().max_animated_elements <= 4);

    app.handle(shaer::InputAction::Up);
    app.handle(shaer::InputAction::Confirm);
    assert(app.audio_settings().replaygain_mode == shaer::ReplayGainMode::Track);

    app.handle(shaer::InputAction::Up);
    app.handle(shaer::InputAction::Confirm);
    assert(app.audio_settings().crossfade_seconds == 3);
}

void archive_quality_reserves_animation_budget_without_battery_saver() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenSettings);
    app.handle(shaer::InputAction::Down);
    app.handle(shaer::InputAction::Down);
    app.handle(shaer::InputAction::Down);
    app.handle(shaer::InputAction::Confirm);
    assert(app.audio_settings().quality_mode == shaer::QualityMode::ArchiveQuality);
    assert(app.power_profile().mode == shaer::PowerMode::Normal);
    assert(app.power_profile().display_budget_fps == 24);
    assert(app.animation_policy().target_fps == 24);
    assert(app.animation_policy().max_animated_elements <= 4);
    assert(!app.animation_policy().reduce_motion);
}

void library_selection_picks_different_tracks() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenLibrary);
    assert(app.firmware_state() == shaer::FirmwareState::LocalLibrary);
    app.handle(shaer::InputAction::Down);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::NowPlaying);
    assert(app.firmware_state() == shaer::FirmwareState::Playback);
    assert(app.playback_snapshot().track.title == "Voice Memory");
}

void spotify_disconnect_stops_then_ok_opens_library() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::StartSpotify);
    assert(app.playback_source() == shaer::PlaybackSource::Spotify);
    assert(app.playback_snapshot().state == shaer::PlaybackState::Playing);
    assert(app.playback_snapshot().track.album == "Spotify Session");

    app.handle(shaer::InputAction::SimulateSpotifyDisconnect);
    assert(app.screen() == shaer::Screen::Popup);
    assert(app.firmware_state() == shaer::FirmwareState::Popup);
    assert(app.playback_state() == shaer::PlaybackState::Stopped);
    assert(app.playback_source() == shaer::PlaybackSource::None);
    assert(app.playback_snapshot().queue_size == 0);
    assert(app.connection_state() == shaer::ConnectionState::SpotifyRecovering);
    assert(app.transition_plan().blocks_input);
    assert(app.transition_plan().reason == "spotify-recovery");
    assert(app.popup().confirm_action == "open_local_library");

    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::Library);
    assert(app.firmware_state() == shaer::FirmwareState::LocalLibrary);
    assert(app.navigation_depth() == 1);
    assert(app.connection_state() == shaer::ConnectionState::LocalOffline);
    assert(app.transition_plan().reason == "recovery-root");

    app.handle(shaer::InputAction::Back);
    assert(app.screen() == shaer::Screen::Home);
    assert(app.navigation_depth() == 1);
}

void low_battery_returns_to_previous_screen() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenLibrary);
    app.handle(shaer::InputAction::SimulateLowBattery);
    assert(app.screen() == shaer::Screen::Popup);
    assert(app.firmware_state() == shaer::FirmwareState::Popup);
    assert(app.animation_policy().reduce_motion);
    assert(app.transition_plan().blocks_input);
    assert(app.transition_plan().power_saving);
    assert(app.transition_plan().duration_ms <= 90);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::Library);
    assert(app.firmware_state() == shaer::FirmwareState::LocalLibrary);
}

void low_battery_reduces_animation_budget() {
    Fixture fixture;
    fixture.battery.percent_ = 10;
    auto app = fixture.make_app();
    app.boot();
    assert(app.animation_policy().reduce_motion);
    assert(app.animation_policy().target_fps <= 12);
    assert(app.animation_policy().max_animated_elements <= 2);
}

void theme_engine_exposes_preferences_not_battery_policy() {
    shaer::ThemeEngine themes;
    themes.set_theme("indian_raga");
    const auto animation = themes.animation_policy();
    const auto transition = themes.transition_plan(shaer::Screen::Home, shaer::Screen::Library, false, "test");
    assert(!animation.reduce_motion);
    assert(!transition.power_saving);
    assert(transition.duration_ms == 240);
}

void theme_cycle_changes_world_profile() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    const auto first = app.theme_profile();
    app.handle(shaer::InputAction::CycleTheme);
    const auto second = app.theme_profile();
    assert(first.id != second.id);
    assert(first.layout_signature != second.layout_signature);
    assert(first.transition_signature != second.transition_signature);
}

void all_theme_profiles_are_distinct() {
    shaer::ThemeEngine themes;
    std::set<std::string> ids;
    for (const auto& id : themes.available_theme_ids()) {
        themes.set_theme(id);
        auto profile = themes.render_profile();
        ids.insert(profile.id);
        assert(!profile.display_name.empty());
        assert(!profile.definition.typography.primary.empty());
        assert(profile.definition.animations.target_fps > 0);
        assert(profile.definition.manifest.theme_version > 0);
        assert(profile.definition.manifest.mandatory_screens.size() >= 17);
        assert(!profile.definition.manifest.preview_image.empty());
    }
    assert(ids.size() == themes.available_theme_ids().size());
}

void all_theme_screen_blueprints_keep_common_behavior() {
    shaer::ThemeEngine themes;
    std::set<std::string> now_playing_keys;
    std::set<std::string> library_keys;
    for (const auto& id : themes.available_theme_ids()) {
        themes.set_theme(id);
        const auto now = themes.screen_blueprint(shaer::Screen::NowPlaying);
        const auto lib = themes.screen_blueprint(shaer::Screen::Library);
        const auto settings = themes.screen_blueprint(shaer::Screen::Settings);

        now_playing_keys.insert(now.chrome + "|" + now.primary_region + "|" + now.selector);
        library_keys.insert(lib.chrome + "|" + lib.primary_region + "|" + lib.selector);
        assert(now.primary_region != lib.primary_region);
        assert(lib.primary_region != settings.primary_region);
    }
    assert(now_playing_keys.size() == 1);
    assert(library_keys.size() == 1);
}

void spotify_recovery_sets_connection_hint() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::StartSpotify);
    app.handle(shaer::InputAction::SimulateSpotifyDisconnect);
    assert(!fixture.display.frames.empty());
    assert(fixture.display.frames.back().connection_hint == "Spotify connection lost -> Local Library handoff");
    app.handle(shaer::InputAction::Confirm);
    assert(fixture.display.frames.back().connection_hint == "Local Library ready offline");
}

void transition_plan_tracks_navigation_and_back() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenLibrary);
    assert(app.transition_plan().from == shaer::Screen::Home);
    assert(app.transition_plan().to == shaer::Screen::Library);
    assert(app.transition_plan().reason == "navigation");
    assert(!app.transition_plan().blocks_input);

    app.handle(shaer::InputAction::Back);
    assert(app.transition_plan().from == shaer::Screen::Library);
    assert(app.transition_plan().to == shaer::Screen::Home);
    assert(app.transition_plan().reason == "back");
}

void play_from_library_opens_now_playing() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenLibrary);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::NowPlaying);
    assert(app.playback_state() == shaer::PlaybackState::Playing);
    assert(app.playback_source() == shaer::PlaybackSource::Local);
    assert(app.playback_snapshot().track.title == "Demo Track");
    assert(app.playback_snapshot().queue_size == 3);
}

void playback_pause_is_owned_by_app_snapshot() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::PlayPause);
    assert(app.playback_snapshot().state == shaer::PlaybackState::Playing);
    app.handle(shaer::InputAction::PlayPause);
    assert(app.playback_snapshot().state == shaer::PlaybackState::Paused);
    app.handle(shaer::InputAction::PlayPause);
    assert(app.playback_snapshot().state == shaer::PlaybackState::Playing);
}

void marginalia_restores_playback_and_previous_focus() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();
    app.handle(shaer::InputAction::OpenLibrary);
    app.handle(shaer::InputAction::Down);
    app.handle(shaer::InputAction::Confirm);
    assert(app.screen() == shaer::Screen::NowPlaying);
    assert(app.playback_state() == shaer::PlaybackState::Playing);

    app.handle(shaer::InputAction::OpenMarginalia);
    assert(app.screen() == shaer::Screen::Marginalia);
    assert(app.firmware_state() == shaer::FirmwareState::Marginalia);
    assert(app.playback_state() == shaer::PlaybackState::Playing);
    assert(app.playback_snapshot().track.id == "local:Voice Memory");
    const auto strokes_before = app.notebook().pages.front().strokes.size();
    assert(app.record_stroke({10, 12, 20, 24, 2, 7, 0, 1234}));
    assert(app.notebook().pages.front().strokes.size() == strokes_before + 1);

    app.handle(shaer::InputAction::Back);
    assert(app.screen() == shaer::Screen::NowPlaying);
    app.handle(shaer::InputAction::Back);
    assert(app.screen() == shaer::Screen::Library);
    assert(app.selected_index() == 1);
    assert(app.playback_state() == shaer::PlaybackState::Playing);
}

void power_button_actions_match_hardware_freeze() {
    Fixture fixture;
    auto app = fixture.make_app();
    app.boot();

    app.handle(shaer::InputAction::ToggleBatterySaver);
    assert(app.power_profile().mode == shaer::PowerMode::BatterySaver);
    assert(app.animation_policy().target_fps <= 12);

    app.handle(shaer::InputAction::EnterSleep);
    assert(app.screen() == shaer::Screen::Sleep);
    assert(app.firmware_state() == shaer::FirmwareState::Sleep);
    app.handle(shaer::InputAction::ToggleBatterySaver);
    assert(app.screen() == shaer::Screen::Home);
    assert(app.firmware_state() == shaer::FirmwareState::Home);

    Fixture shutdown_fixture;
    auto shutdown_app = shutdown_fixture.make_app();
    shutdown_app.boot();
    shutdown_app.handle(shaer::InputAction::PlayPause);
    assert(shutdown_app.playback_state() == shaer::PlaybackState::Playing);
    shutdown_app.handle(shaer::InputAction::BeginShutdown);
    assert(shutdown_app.screen() == shaer::Screen::Shutdown);
    assert(shutdown_app.firmware_state() == shaer::FirmwareState::Shutdown);
    assert(shutdown_app.playback_state() == shaer::PlaybackState::Stopped);
}

}  // namespace

int main() {
    boot_opens_home();
    back_uses_bounded_clean_stack();
    home_selection_opens_expected_screens();
    selection_is_clamped_per_screen();
    settings_selection_is_clamped_and_updates_audio_power();
    archive_quality_reserves_animation_budget_without_battery_saver();
    library_selection_picks_different_tracks();
    spotify_disconnect_stops_then_ok_opens_library();
    low_battery_returns_to_previous_screen();
    low_battery_reduces_animation_budget();
    theme_engine_exposes_preferences_not_battery_policy();
    theme_cycle_changes_world_profile();
    all_theme_profiles_are_distinct();
    all_theme_screen_blueprints_keep_common_behavior();
    spotify_recovery_sets_connection_hint();
    transition_plan_tracks_navigation_and_back();
    play_from_library_opens_now_playing();
    playback_pause_is_owned_by_app_snapshot();
    marginalia_restores_playback_and_previous_focus();
    power_button_actions_match_hardware_freeze();
    std::cout << "navigation_tests passed\n";
    return 0;
}
