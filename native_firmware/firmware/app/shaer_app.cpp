#include "shaer_app.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

namespace shaer {

namespace {

Track make_track(std::string title, std::string artist, std::string album, std::string file_path, PlaybackSource source) {
    Track track;
    track.id = file_path.empty() ? "local:" + title : "local:" + file_path;
    track.title = std::move(title);
    track.artist = std::move(artist);
    track.album = std::move(album);
    track.file_path = std::move(file_path);
    track.source = source;
    return track;
}

}  // namespace

ShaerApp::ShaerApp(Hardware hardware) : hw_(hardware) {}

void ShaerApp::boot() {
    refresh_power_profile("boot");
    navigation_stack_.clear();
    navigate_to(Screen::Home);
    push_log("[Boot] anticipation and curiosity");
    render();
}

void ShaerApp::apply_runtime_settings(const RuntimeSettings& settings) {
    theme_.set_theme(settings.active_theme);
    volume_ = std::clamp(settings.volume, 0, 100);
    audio_settings_.crossfade_seconds = std::clamp(settings.crossfade_seconds, 0, 12);

    if (settings.replaygain_mode == "track") {
        audio_settings_.replaygain_mode = ReplayGainMode::Track;
    } else if (settings.replaygain_mode == "album") {
        audio_settings_.replaygain_mode = ReplayGainMode::Album;
    } else {
        audio_settings_.replaygain_mode = ReplayGainMode::Off;
    }

    audio_settings_.quality_mode = settings.quality_mode == "archive_quality"
        ? QualityMode::ArchiveQuality
        : QualityMode::Balanced;

    if (settings.power_mode == "battery_saver") {
        power_profile_.mode = PowerMode::BatterySaver;
    } else {
        power_profile_.mode = PowerMode::Normal;
    }
    refresh_power_profile("settings loaded");
    push_log("[Settings] runtime settings loaded from SQLite");
}

void ShaerApp::show_diagnostic_popup(const std::string& title, const std::string& body) {
    previous_screen_ = screen_;
    set_screen_direct(Screen::Popup, true, "boot-diagnostics");
    popup_ = {
        title,
        body,
        true,
        "return_previous",
    };
    push_log("[Boot] diagnostics popup opened");
    render();
}

void ShaerApp::handle(InputAction action) {
    if (action == InputAction::Quit) {
        return;
    }

    if (screen_ == Screen::Popup) {
        if (action == InputAction::Confirm) {
            confirm_popup();
        } else if (action == InputAction::Back) {
            cancel_popup();
        } else {
            push_log("[UI] blocking popup is waiting for OK");
        }
        render();
        return;
    }

    switch (action) {
        case InputAction::Confirm:
            if (screen_ == Screen::Home) {
                if (selected_index_ == 0) {
                    open_library();
                } else if (selected_index_ == 1) {
                    navigate_to(Screen::BluetoothConnect);
                    push_log("[Navigation] Spotify Connect pairing opened");
                } else if (selected_index_ == 2) {
                    navigate_to(Screen::VoiceArchive);
                    push_log("[Navigation] Voice Memos opened");
                } else {
                    open_settings();
                }
            } else if (screen_ == Screen::Library) {
                const char* title = selected_index_ == 1 ? "Voice Memory" : selected_index_ == 2 ? "FLAC Test" : "Demo Track";
                start_local_playback(make_track(title, "SHAeR Local Archive", "Local Archive", "", PlaybackSource::Local));
                open_now_playing();
            } else if (screen_ == Screen::NowPlaying) {
                open_marginalia();
            } else if (screen_ == Screen::Settings) {
                activate_selected_setting();
            } else if (screen_ == Screen::BluetoothConnect) {
                previous_screen_ = screen_;
                set_screen_direct(Screen::Popup, true, "spotify-device-detected");
                popup_ = {
                    "SPOTIFY FOUND",
                    "MAIN PHONE READY",
                    true,
                    "return_previous",
                };
                push_log("[Spotify] Connect device detected popup opened");
            }
            break;
        case InputAction::Back:
            navigate_back();
            break;
        case InputAction::Up:
            selected_index_ = std::max(0, selected_index_ - 1);
            scroll_offset_ = std::min(scroll_offset_, selected_index_);
            push_log("[Input] encoder up");
            break;
        case InputAction::Down:
            selected_index_ = std::min(selectable_count_for(screen_) - 1, selected_index_ + 1);
            scroll_offset_ = std::max(scroll_offset_, std::max(0, selected_index_ - 5));
            push_log("[Input] encoder down");
            break;
        case InputAction::PlayPause:
            toggle_playback();
            break;
        case InputAction::Next:
            start_local_playback(make_track("Next Demo Track", "SHAeR Local Archive", "Local Archive", "", PlaybackSource::Local));
            open_now_playing();
            push_log("[Audio] next local track");
            break;
        case InputAction::Previous:
            start_local_playback(make_track("Previous Demo Track", "SHAeR Local Archive", "Local Archive", "", PlaybackSource::Local));
            open_now_playing();
            push_log("[Audio] previous local track");
            break;
        case InputAction::SeekForward:
            playback_.progress_seconds = std::min(playback_.duration_seconds, playback_.progress_seconds + 10);
            push_log("[Audio] seek forward");
            break;
        case InputAction::SeekBackward:
            playback_.progress_seconds = std::max(0, playback_.progress_seconds - 10);
            push_log("[Audio] seek backward");
            break;
        case InputAction::VolumeUp:
            volume_ = std::min(100, volume_ + 2);
            push_log("[Audio] volume up");
            break;
        case InputAction::VolumeDown:
            volume_ = std::max(0, volume_ - 2);
            push_log("[Audio] volume down");
            break;
        case InputAction::OpenLibrary:
            open_library();
            break;
        case InputAction::OpenMarginalia:
            open_marginalia();
            break;
        case InputAction::OpenSettings:
            open_settings();
            break;
        case InputAction::StartSpotify:
            wifi_connected_ = true;
            start_spotify_playback(make_track("Cloud Song", "Spotify Connect", "Spotify Session", "", PlaybackSource::Spotify));
            open_now_playing();
            push_log("[Spotify] Connect playback started");
            break;
        case InputAction::CycleTheme:
            cycle_theme();
            break;
        case InputAction::SimulateSpotifyDisconnect:
            spotify_disconnect();
            break;
        case InputAction::SimulateBluetoothDisconnect:
            bluetooth_disconnect();
            break;
        case InputAction::SimulateLowBattery:
            low_battery();
            break;
        case InputAction::ToggleBatterySaver:
            toggle_battery_saver();
            break;
        case InputAction::EnterSleep:
            enter_sleep();
            break;
        case InputAction::BeginShutdown:
            begin_shutdown();
            break;
        case InputAction::Reboot:
            begin_shutdown();
            push_log("[Power] reboot requested");
            break;
        case InputAction::None:
            push_log("[Input] ignored unknown command");
            break;
        case InputAction::Quit:
            break;
    }

    render();
}

void ShaerApp::select_theme(const std::string& id) {
    theme_.set_theme(id);
    push_log("[Theme] selected " + id);
    render();
}

bool ShaerApp::record_stroke(const NotebookPage::Stroke& stroke) {
    if (screen_ != Screen::Marginalia || notebook_.pages.empty()) {
        push_log("[Marginalia] stylus input ignored outside notebook");
        return false;
    }
    const auto page_index = std::clamp(selected_index_, 0, static_cast<int>(notebook_.pages.size()) - 1);
    auto& page = notebook_.pages[static_cast<size_t>(page_index)];
    page.strokes.push_back(stroke);
    ++page.revision;
    page.modified_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    notebook_.modified_at = page.modified_at;
    notebook_.page_count = static_cast<int>(notebook_.pages.size());
    const bool saved = notebook_store_.save(notebook_);
    push_log(saved ? "[Marginalia] stroke autosaved" : "[Marginalia] stroke autosave failed");
    return saved;
}

void ShaerApp::render() {
    hw_.display->render(model());
}

FirmwareState ShaerApp::firmware_state() const {
    return firmware_state_;
}

Screen ShaerApp::screen() const {
    return screen_;
}

int ShaerApp::navigation_depth() const {
    return static_cast<int>(navigation_stack_.size());
}

int ShaerApp::selected_index() const {
    return selected_index_;
}

AnimationPolicy ShaerApp::animation_policy() const {
    const bool power_sensitive_screen = screen_ == Screen::Popup && popup_.confirm_action == "return_previous";
    auto policy = theme_.animation_policy();
    const bool budget_reduced =
        power_profile_.display_budget_fps < policy.target_fps ||
        power_profile_.max_animated_elements < policy.max_animated_elements;

    policy.target_fps = std::min(policy.target_fps, power_profile_.display_budget_fps);
    policy.max_animated_elements = std::min(policy.max_animated_elements, power_profile_.max_animated_elements);

    if (force_power_saving() || power_sensitive_screen || budget_reduced) {
        policy.rich_transitions = false;
        policy.reduce_motion = force_power_saving() || power_sensitive_screen;
        policy.reason = power_sensitive_screen && !force_power_saving()
            ? "power-sensitive screen"
            : power_profile_.reason;
    }
    return policy;
}

TransitionPlan ShaerApp::transition_plan() const {
    return transition_;
}

ConnectionState ShaerApp::connection_state() const {
    return connection_state_;
}

AudioSettings ShaerApp::audio_settings() const {
    return audio_settings_;
}

PowerProfile ShaerApp::power_profile() const {
    return power_profile_;
}

ThemeRenderProfile ShaerApp::theme_profile() const {
    return theme_.render_profile();
}

PlaybackState ShaerApp::playback_state() const {
    return playback_.state;
}

PlaybackSource ShaerApp::playback_source() const {
    return playback_.source;
}

PlaybackSnapshot ShaerApp::playback_snapshot() const {
    return playback_;
}

const Notebook& ShaerApp::notebook() const {
    return notebook_;
}

Notification ShaerApp::popup() const {
    return popup_;
}

const std::deque<std::string>& ShaerApp::log() const {
    return log_;
}

void ShaerApp::open_library() {
    navigate_to(Screen::Library);
    push_log("[Navigation] Local Library opened");
}

void ShaerApp::open_now_playing() {
    navigate_to(Screen::NowPlaying);
    push_log("[Navigation] Now Playing opened");
}

void ShaerApp::open_marginalia() {
    if (playback_.track.id.empty() && playback_.track.title.empty()) {
        push_log("[Marginalia] unavailable without an active song");
        return;
    }
    notebook_ = notebook_store_.load_or_create(playback_.track);
    if (notebook_.pages.empty()) {
        NotebookPage page;
        page.id = notebook_.id + ":page001";
        page.created_at = notebook_.created_at;
        page.modified_at = notebook_.modified_at;
        notebook_.pages.push_back(std::move(page));
        notebook_.page_count = 1;
        notebook_store_.save(notebook_);
    }
    playback_.track.notes_available = !notebook_.pages.empty();
    navigate_to(Screen::Marginalia);
    push_log("[Marginalia] contextual notebook opened");
}

void ShaerApp::open_settings() {
    navigate_to(Screen::Settings);
    push_log("[Navigation] Settings opened");
}

void ShaerApp::open_about() {
    navigate_to(Screen::About);
    push_log("[Navigation] About opened");
}

void ShaerApp::navigate_to(Screen screen) {
    if (!navigation_stack_.empty() && navigation_stack_.back().screen == screen) {
        screen_ = screen;
        return;
    }
    const Screen from = screen_;
    if (navigation_stack_.size() >= 8) {
        navigation_stack_.pop_front();
    }
    if (!navigation_stack_.empty()) {
        navigation_stack_.back().selected_index = selected_index_;
        navigation_stack_.back().scroll_offset = scroll_offset_;
    }
    navigation_stack_.push_back({screen, 0, 0});
    screen_ = screen;
    transition_firmware_state(state_for_screen(screen), "screen navigation");
    selected_index_ = 0;
    scroll_offset_ = 0;
    transition_ = theme_.transition_plan(from, screen, false, "navigation");
    if (force_power_saving()) {
        transition_.style = "reduced " + transition_.style;
        transition_.duration_ms = 90;
        transition_.power_saving = true;
    }
}

void ShaerApp::reset_navigation_to(Screen screen) {
    const Screen from = screen_;
    navigation_stack_.clear();
    navigation_stack_.push_back({screen, 0, 0});
    screen_ = screen;
    transition_firmware_state(state_for_screen(screen), "navigation reset");
    selected_index_ = 0;
    scroll_offset_ = 0;
    transition_ = theme_.transition_plan(from, screen, false, "recovery-root");
    if (force_power_saving()) {
        transition_.style = "reduced " + transition_.style;
        transition_.duration_ms = 90;
        transition_.power_saving = true;
    }
}

void ShaerApp::set_screen_direct(Screen screen, bool blocks_input, std::string reason) {
    const Screen from = screen_;
    screen_ = screen;
    transition_firmware_state(state_for_screen(screen), reason);
    transition_ = theme_.transition_plan(from, screen, blocks_input, std::move(reason));
    if (force_power_saving()) {
        transition_.style = "reduced " + transition_.style;
        transition_.duration_ms = 90;
        transition_.power_saving = true;
    }
}

void ShaerApp::navigate_back() {
    if (navigation_stack_.size() > 1) {
        if (screen_ == Screen::Marginalia) save_notebook("leaving Marginalia");
        const Screen from = screen_;
        navigation_stack_.pop_back();
        const auto& restored = navigation_stack_.back();
        screen_ = restored.screen;
        selected_index_ = restored.selected_index;
        scroll_offset_ = restored.scroll_offset;
        transition_firmware_state(state_for_screen(screen_), "back");
        clamp_selection();
        transition_ = theme_.transition_plan(from, screen_, false, "back");
        if (force_power_saving()) {
            transition_.style = "reduced " + transition_.style;
            transition_.duration_ms = 90;
            transition_.power_saving = true;
        }
        push_log("[Navigation] back");
        return;
    }
    const Screen from = screen_;
    screen_ = Screen::Home;
    transition_firmware_state(FirmwareState::Home, "back root");
    if (navigation_stack_.empty()) {
        navigation_stack_.push_back({Screen::Home, 0, 0});
    } else {
        navigation_stack_.back() = {Screen::Home, 0, 0};
    }
    selected_index_ = 0;
    scroll_offset_ = 0;
    push_log("[Navigation] already at Home");
    clamp_selection();
    transition_ = theme_.transition_plan(from, screen_, false, "back-root");
    if (force_power_saving()) {
        transition_.style = "reduced " + transition_.style;
        transition_.duration_ms = 90;
        transition_.power_saving = true;
    }
}

bool ShaerApp::transition_firmware_state(FirmwareState next, std::string reason) {
    if (firmware_state_ == next) {
        return true;
    }
    if (!firmware_transition_allowed(firmware_state_, next)) {
        push_log(std::string("[State] blocked ") + to_string(firmware_state_) + " -> " + to_string(next));
        return false;
    }
    firmware_state_ = next;
    push_log(std::string("[State] ") + to_string(next) + " (" + reason + ")");
    return true;
}

bool ShaerApp::firmware_transition_allowed(FirmwareState from, FirmwareState to) const {
    if (from == to) return true;
    if (from == FirmwareState::Booting) return to == FirmwareState::Home || to == FirmwareState::Popup;
    if (to == FirmwareState::Popup) return true;
    if (from == FirmwareState::Popup) {
        return to == FirmwareState::Home || to == FirmwareState::LocalLibrary ||
               to == FirmwareState::Playback || to == FirmwareState::Marginalia || to == FirmwareState::Settings ||
               to == FirmwareState::Shutdown;
    }
    if (to == FirmwareState::Sleep || to == FirmwareState::Charging || to == FirmwareState::Shutdown) {
        return true;
    }
    if (from == FirmwareState::Sleep || from == FirmwareState::Charging) {
        return to == FirmwareState::Home || to == FirmwareState::Popup;
    }
    return to == FirmwareState::Home || to == FirmwareState::LocalLibrary ||
           to == FirmwareState::Playback || to == FirmwareState::Marginalia || to == FirmwareState::Settings;
}

FirmwareState ShaerApp::state_for_screen(Screen screen) const {
    switch (screen) {
        case Screen::Boot: return FirmwareState::Booting;
        case Screen::Home: return FirmwareState::Home;
        case Screen::Library: return FirmwareState::LocalLibrary;
        case Screen::NowPlaying: return FirmwareState::Playback;
        case Screen::Marginalia: return FirmwareState::Marginalia;
        case Screen::VoiceArchive: return FirmwareState::LocalLibrary;
        case Screen::BluetoothConnect: return FirmwareState::Home;
        case Screen::Settings: return FirmwareState::Settings;
        case Screen::About: return FirmwareState::Settings;
        case Screen::Popup: return FirmwareState::Popup;
        case Screen::Aod: return FirmwareState::Sleep;
        case Screen::Charging: return FirmwareState::Charging;
        case Screen::Sleep: return FirmwareState::Sleep;
        case Screen::Shutdown: return FirmwareState::Shutdown;
    }
    return FirmwareState::Home;
}

void ShaerApp::toggle_playback() {
    if (playback_.state == PlaybackState::Stopped) {
        start_local_playback(make_track("Demo Track", "SHAeR Local Archive", "Local Archive", "", PlaybackSource::Local));
        open_now_playing();
        push_log("[Audio] local playback started");
    } else {
        toggle_playback_pause();
    }
}

void ShaerApp::start_local_playback(Track track) {
    track.source = PlaybackSource::Local;
    hw_.audio->play_local(track);
    playback_.state = PlaybackState::Playing;
    playback_.source = PlaybackSource::Local;
    playback_.track = track;
    playback_.progress_seconds = 0;
    playback_.duration_seconds = track.title == "Voice Memory" ? 42 : 184;
    playback_.queue_index = 1;
    playback_.queue_size = 3;
    connection_state_ = ConnectionState::Ready;
    connection_hint_ = "Local playback active";
}

void ShaerApp::start_spotify_playback(Track track) {
    track.source = PlaybackSource::Spotify;
    hw_.audio->play_spotify(track);
    playback_.state = PlaybackState::Playing;
    playback_.source = PlaybackSource::Spotify;
    playback_.track = track;
    playback_.progress_seconds = 0;
    playback_.duration_seconds = 215;
    playback_.queue_index = 1;
    playback_.queue_size = 1;
    connection_state_ = ConnectionState::SpotifyActive;
    connection_hint_ = "Spotify Connect session active";
}

void ShaerApp::stop_playback(std::string reason) {
    hw_.audio->stop();
    playback_.state = PlaybackState::Stopped;
    playback_.source = PlaybackSource::None;
    playback_.progress_seconds = 0;
    playback_.queue_index = 0;
    playback_.queue_size = 0;
    push_log("[Audio] stopped: " + reason);
}

void ShaerApp::toggle_playback_pause() {
    hw_.audio->pause();
    playback_.state = playback_.state == PlaybackState::Playing
        ? PlaybackState::Paused
        : PlaybackState::Playing;
    push_log("[Audio] play/pause toggled");
}

void ShaerApp::spotify_disconnect() {
    wifi_connected_ = false;
    connection_hint_ = "Spotify connection lost -> Local Library handoff";
    connection_state_ = ConnectionState::SpotifyRecovering;
    stop_playback("spotify connection lost");
    previous_screen_ = screen_;
    set_screen_direct(Screen::Popup, true, "spotify-recovery");
    popup_ = {
        "Signal slipped",
        "Your local archive is ready.",
        true,
        "open_local_library",
    };
    push_log("[Spotify] WiFi dropped mid-song; playback stopped");
}

void ShaerApp::bluetooth_disconnect() {
    if (playback_.source == PlaybackSource::Bluetooth) {
        stop_playback("bluetooth disconnected");
    }
    connection_hint_ = "Bluetooth disconnected -> choose output";
    connection_state_ = ConnectionState::BluetoothRecovering;
    previous_screen_ = screen_;
    set_screen_direct(Screen::Popup, true, "bluetooth-recovery");
    popup_ = {
        "Bluetooth wandered off",
        "Playback paused until you choose an output.",
        true,
        "return_previous",
    };
    push_log("[Bluetooth] disconnect handled");
}

void ShaerApp::low_battery() {
    battery_override_percent_ = 10;
    connection_state_ = ConnectionState::LowPower;
    power_profile_.mode = PowerMode::Critical;
    refresh_power_profile("low battery");
    previous_screen_ = screen_;
    set_screen_direct(Screen::Popup, true, "low-power-warning");
    popup_ = {
        "Charge me soon",
        "Battery is getting low.",
        true,
        "return_previous",
    };
    push_log("[Power] low battery warning");
}

void ShaerApp::toggle_battery_saver() {
    if (screen_ == Screen::Sleep || screen_ == Screen::Aod) {
        const Screen wake_target = previous_screen_ == Screen::Sleep || previous_screen_ == Screen::Aod
            ? Screen::Home
            : previous_screen_;
        reset_navigation_to(wake_target);
        push_log("[Power] woke from power button");
        return;
    }

    power_profile_.mode = power_profile_.mode == PowerMode::BatterySaver
        ? PowerMode::Normal
        : PowerMode::BatterySaver;
    refresh_power_profile("power button short press");
    push_log(std::string("[Power] battery saver ") + to_string(power_profile_.mode));
}

void ShaerApp::enter_sleep() {
    if (screen_ == Screen::Marginalia) save_notebook("before sleep");
    previous_screen_ = screen_;
    set_screen_direct(Screen::Sleep, true, "power button double press");
    push_log("[Power] sleep entered; idle shutdown timer is 45 minutes");
}

void ShaerApp::begin_shutdown() {
    if (screen_ == Screen::Marginalia) save_notebook("before power-off");
    previous_screen_ = screen_;
    set_screen_direct(Screen::Shutdown, true, "power button long press");
    stop_playback("shutdown requested");
    push_log("[Power] slow goodbye; Linux shutdown may follow");
}

void ShaerApp::confirm_popup() {
    push_log("[UI] popup confirmed");
    if (popup_.confirm_action == "open_local_library") {
        popup_ = {};
        reset_navigation_to(Screen::Library);
        connection_hint_ = "Local Library ready offline";
        connection_state_ = ConnectionState::LocalOffline;
        push_log("[Navigation] Local Library opened as recovery root");
        return;
    }
    popup_ = {};
    screen_ = previous_screen_;
    transition_firmware_state(state_for_screen(screen_), "popup dismiss");
    transition_ = theme_.transition_plan(Screen::Popup, screen_, false, "popup-dismiss");
    if (force_power_saving()) {
        transition_.style = "reduced " + transition_.style;
        transition_.duration_ms = 90;
        transition_.power_saving = true;
    }
}

void ShaerApp::cancel_popup() {
    push_log("[UI] popup cancelled");
    popup_ = {};
    reset_navigation_to(Screen::Home);
}

void ShaerApp::cycle_theme() {
    theme_.cycle_theme();
    push_log("[Theme] switched to " + theme_.active_theme().display_name);
}

void ShaerApp::activate_selected_setting() {
    if (selected_index_ == 0) {
        cycle_theme();
    } else if (selected_index_ == 1) {
        cycle_crossfade();
    } else if (selected_index_ == 2) {
        cycle_replaygain();
    } else if (selected_index_ == 3) {
        cycle_quality();
    } else {
        cycle_power_mode();
    }
}

void ShaerApp::cycle_crossfade() {
    if (audio_settings_.crossfade_seconds == 0) {
        audio_settings_.crossfade_seconds = 3;
    } else if (audio_settings_.crossfade_seconds == 3) {
        audio_settings_.crossfade_seconds = 6;
    } else if (audio_settings_.crossfade_seconds == 6) {
        audio_settings_.crossfade_seconds = 12;
    } else {
        audio_settings_.crossfade_seconds = 0;
    }
    push_log("[Settings] crossfade " + std::to_string(audio_settings_.crossfade_seconds) + "s");
}

void ShaerApp::cycle_replaygain() {
    if (audio_settings_.replaygain_mode == ReplayGainMode::Off) {
        audio_settings_.replaygain_mode = ReplayGainMode::Track;
    } else if (audio_settings_.replaygain_mode == ReplayGainMode::Track) {
        audio_settings_.replaygain_mode = ReplayGainMode::Album;
    } else {
        audio_settings_.replaygain_mode = ReplayGainMode::Off;
    }
    push_log(std::string("[Settings] replaygain ") + to_string(audio_settings_.replaygain_mode));
}

void ShaerApp::cycle_quality() {
    audio_settings_.quality_mode = audio_settings_.quality_mode == QualityMode::Balanced
        ? QualityMode::ArchiveQuality
        : QualityMode::Balanced;
    refresh_power_profile("quality mode changed");
    push_log(std::string("[Settings] quality ") + to_string(audio_settings_.quality_mode));
}

void ShaerApp::cycle_power_mode() {
    power_profile_.mode = power_profile_.mode == PowerMode::Normal
        ? PowerMode::BatterySaver
        : PowerMode::Normal;
    refresh_power_profile("manual power mode");
    push_log(std::string("[Settings] power ") + to_string(power_profile_.mode));
}

void ShaerApp::push_log(std::string message) {
    log_.push_back(std::move(message));
    while (log_.size() > 6) {
        log_.pop_front();
    }
}

void ShaerApp::save_notebook(const std::string& reason) {
    if (notebook_.song_id.empty()) return;
    notebook_.modified_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    notebook_.page_count = static_cast<int>(notebook_.pages.size());
    if (notebook_store_.save(notebook_)) {
        push_log("[Marginalia] autosaved " + reason);
    } else {
        push_log("[Marginalia] autosave failed " + reason);
    }
}

RenderModel ShaerApp::model() const {
    RenderModel render;
    render.firmware_state = firmware_state_;
    render.screen = screen_;
    render.theme = theme_.render_profile();
    render.blueprint = theme_.screen_blueprint(screen_);
    render.animation = animation_policy();
    render.transition = transition_;
    render.tick_count = static_cast<int>(log_.size());
    render.battery_percent = hw_.battery->percent();
    render.battery_percent = effective_battery_percent();
    render.charging = false;
    render.wifi_connected = wifi_connected_;
    render.bluetooth_connected = hw_.bluetooth->connected();
    render.playback = playback_;
    render.selected_index = selected_index_;
    render.scroll_offset = scroll_offset_;
    render.connection_hint = connection_hint_;
    render.connection_state = connection_state_;
    render.audio_settings = audio_settings_;
    render.power = power_profile_;
    render.volume = volume_;
    render.popup = popup_;
    render.notebook = notebook_;
    render.console.assign(log_.begin(), log_.end());
    return render;
}

int ShaerApp::selectable_count_for(Screen screen) const {
    if (screen == Screen::Home) return 4;
    if (screen == Screen::Library) return 3;
    if (screen == Screen::Marginalia) return 1;
    if (screen == Screen::VoiceArchive) return 3;
    if (screen == Screen::BluetoothConnect) return 1;
    if (screen == Screen::Settings) return 5;
    return 1;
}

void ShaerApp::clamp_selection() {
    selected_index_ = std::clamp(selected_index_, 0, selectable_count_for(screen_) - 1);
}

int ShaerApp::effective_battery_percent() const {
    if (battery_override_percent_ >= 0) {
        return battery_override_percent_;
    }
    return hw_.battery->percent();
}

bool ShaerApp::force_power_saving() const {
    return power_profile_.mode == PowerMode::BatterySaver || power_profile_.mode == PowerMode::Critical;
}

void ShaerApp::refresh_power_profile(std::string reason) {
    if (effective_battery_percent() <= 15) {
        power_profile_.mode = PowerMode::Critical;
    }

    if (power_profile_.mode == PowerMode::Critical) {
        power_profile_.display_budget_fps = 12;
        power_profile_.max_animated_elements = 2;
        power_profile_.wifi_power_save = true;
        power_profile_.bluetooth_idle_allowed = false;
        power_profile_.reason = reason == "boot" ? "critical battery" : reason;
        return;
    }

    if (power_profile_.mode == PowerMode::BatterySaver) {
        power_profile_.display_budget_fps = 12;
        power_profile_.max_animated_elements = 2;
        power_profile_.wifi_power_save = true;
        power_profile_.bluetooth_idle_allowed = false;
        power_profile_.reason = reason == "boot" ? "battery saver" : reason;
        return;
    }

    power_profile_.display_budget_fps = audio_settings_.quality_mode == QualityMode::ArchiveQuality ? 24 : 30;
    power_profile_.max_animated_elements = audio_settings_.quality_mode == QualityMode::ArchiveQuality ? 4 : 6;
    power_profile_.wifi_power_save = false;
    power_profile_.bluetooth_idle_allowed = true;
    power_profile_.reason = audio_settings_.quality_mode == QualityMode::ArchiveQuality
        ? "archive quality reserves CPU"
        : "normal";
}

}  // namespace shaer
