#pragma once

#include <string>
#include <vector>
#include <map>

namespace shaer {

enum class Screen {
    Boot,
    Home,
    Library,
    NowPlaying,
    Marginalia,
    VoiceArchive,
    BluetoothConnect,
    Settings,
    About,
    Popup,
    Aod,
    Charging,
    Sleep,
    Shutdown,
};

enum class FirmwareState {
    Booting,
    Home,
    LocalLibrary,
    Playback,
    Marginalia,
    Settings,
    Popup,
    Sleep,
    Charging,
    Shutdown,
};

enum class PlaybackSource {
    None,
    Local,
    Spotify,
    Bluetooth,
};

enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
};

enum class ConnectionState {
    Ready,
    SpotifyActive,
    SpotifyRecovering,
    BluetoothRecovering,
    LocalOffline,
    WifiRecovering,
    LowPower,
};

enum class ReplayGainMode {
    Off,
    Track,
    Album,
};

enum class QualityMode {
    Balanced,
    ArchiveQuality,
};

enum class PowerMode {
    Normal,
    BatterySaver,
    Critical,
};

enum class InputAction {
    Confirm,
    Back,
    Up,
    Down,
    PlayPause,
    Next,
    Previous,
    SeekForward,
    SeekBackward,
    VolumeUp,
    VolumeDown,
    OpenLibrary,
    OpenMarginalia,
    OpenSettings,
    StartSpotify,
    CycleTheme,
    SimulateSpotifyDisconnect,
    SimulateBluetoothDisconnect,
    SimulateLowBattery,
    ToggleBatterySaver,
    EnterSleep,
    BeginShutdown,
    Reboot,
    Quit,
    None,
};

struct Track {
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::string file_path;
    PlaybackSource source = PlaybackSource::None;
    int track_number = 0;
    std::string genre;
    int duration_seconds = 0;
    std::string codec;
    int bitrate_kbps = 0;
    int sample_rate_hz = 0;
    int bit_depth = 0;
    std::string album_art_path;
    std::string folder;
    std::string album_artist;
    int year = 0;
    int disc_number = 0;
    bool lyrics_available = false;
    bool notes_available = false;
    int play_count = 0;
    long long date_added = 0;
    long long last_played = 0;
    bool favorite = false;
    std::string file_format;
};

using Song = Track;

struct Playlist {
    std::string id;
    std::string name;
    PlaybackSource source = PlaybackSource::None;
    std::string description;
    std::string artwork_path;
    std::vector<Song> songs;
    long long created_at = 0;
    long long modified_at = 0;
};

struct QueueModel {
    std::vector<Song> songs;
    int current_index = 0;
    bool shuffle = false;
    bool repeat = false;
    std::vector<Song> history;
};

struct NotebookPage {
    std::string id;
    std::string drawing_path;
    std::string thumbnail_path;
    long long created_at = 0;
    long long modified_at = 0;
    int revision = 0;
    struct Stroke {
        int start_x = 0;
        int start_y = 0;
        int end_x = 0;
        int end_y = 0;
        int width = 1;
        int colour = 0;
        int pressure = 0;
        long long timestamp = 0;
    };
    std::vector<Stroke> strokes;
};

struct Notebook {
    std::string id;
    std::string song_id;
    int page_count = 0;
    long long created_at = 0;
    long long modified_at = 0;
    PlaybackSource source = PlaybackSource::None;
    std::string title;
    std::string artist;
    std::string album;
    std::string thumbnail_path;
    std::vector<NotebookPage> pages;
};

struct LibraryIndex {
    std::vector<std::string> songs;
    std::vector<std::string> albums;
    std::vector<std::string> artists;
    std::vector<std::string> folders;
    std::vector<std::string> recently_added;
    std::vector<std::string> recently_played;
    std::vector<std::string> favorites;
};

struct PlaybackSnapshot {
    PlaybackState state = PlaybackState::Stopped;
    PlaybackSource source = PlaybackSource::None;
    Track track;
    int progress_seconds = 0;
    int duration_seconds = 0;
    bool shuffle = false;
    bool repeat = false;
    int queue_index = 0;
    int queue_size = 0;
};

struct Notification {
    std::string title;
    std::string body;
    bool blocking = false;
    std::string confirm_action;
};

struct AnimationPolicy {
    int target_fps = 30;
    int max_animated_elements = 6;
    bool rich_transitions = true;
    bool reduce_motion = false;
    std::string reason;
};

struct TransitionPlan {
    Screen from = Screen::Boot;
    Screen to = Screen::Boot;
    std::string style;
    int duration_ms = 0;
    bool blocks_input = false;
    bool power_saving = false;
    std::string reason;
};

struct AudioSettings {
    int crossfade_seconds = 0;
    ReplayGainMode replaygain_mode = ReplayGainMode::Off;
    double replaygain_preamp_db = 0.0;
    bool replaygain_prevent_clipping = true;
    QualityMode quality_mode = QualityMode::Balanced;
};

struct PowerProfile {
    PowerMode mode = PowerMode::Normal;
    int display_budget_fps = 30;
    int max_animated_elements = 6;
    bool wifi_power_save = false;
    bool bluetooth_idle_allowed = true;
    std::string reason = "normal";
};

struct ClockSnapshot {
    std::string time_12h = "--:-- --";
    std::string date_label = "---- -- --";
    std::string source = "system";
    bool valid = false;
};

struct RuntimeSettings {
    std::string active_theme = "archive_dark";
    std::string os_name = u8"आदि Vasi OS";
    std::string music_directory = "/var/lib/shaer/music";
    int volume = 50;
    int crossfade_seconds = 0;
    std::string replaygain_mode = "off";
    std::string quality_mode = "balanced";
    std::string power_mode = "normal";
    std::map<std::string, std::string> values;
};

struct SettingsItem {
    std::string key;
    std::string label;
    std::string value;
    bool editable = true;
    bool placeholder = false;
};

struct SettingsCategory {
    std::string id;
    std::string title;
    std::vector<SettingsItem> items;
};

struct SettingsUiState {
    bool inside_category = false;
    int category_index = 0;
    int item_index = 0;
    std::vector<SettingsCategory> categories;
};

struct DeviceInfoSnapshot {
    std::string firmware_version = "0.1.0-alpha.1";
    std::string build_number = "dev";
    std::string git_commit = "unknown";
    std::string git_tag = "untagged";
    std::string hardware_revision = "V1 breadboard";
    std::string device_name = "SHAeR";
    std::string hostname = "unknown";
    std::string ip_address = "unknown";
    long long storage_free_mb = 0;
    long long storage_used_mb = 0;
    int battery_health_percent = 100;
    int cpu_temp_c = 0;
    int fps = 60;
    int cpu_usage_percent = 0;
    long long memory_used_mb = 0;
};

struct ThemeColor {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

struct ThemePalette {
    ThemeColor background{5, 8, 18};
    ThemeColor foreground{238, 244, 230};
    ThemeColor accent{112, 94, 166};
    ThemeColor selection{112, 94, 166};
    ThemeColor disabled{94, 108, 126};
    ThemeColor popup{16, 22, 36};
    ThemeColor border{112, 94, 166};
    ThemeColor status_bar{16, 22, 36};
    ThemeColor footer{16, 22, 36};
    ThemeColor progress_background{16, 22, 36};
    ThemeColor progress_foreground{112, 94, 166};
    ThemeColor volume_bar{112, 94, 166};
    ThemeColor warning{230, 120, 45};
};

struct ThemeTypography {
    std::string primary = "default_pixel";
    std::string secondary = "default_pixel";
    int heading_size = 2;
    int body_size = 1;
    int small_size = 1;
    int footer_size = 1;
    int line_height = 14;
    int letter_spacing = 0;
};

struct ThemeSpacing {
    int screen_padding = 8;
    int sidebar_width = 34;
    int status_height = 22;
    int footer_height = 18;
    int row_height = 34;
    int popup_width = 184;
    int popup_height = 112;
};

struct ThemeIcons {
    std::string battery = "BT";
    std::string bluetooth = "BL";
    std::string wifi = "WF";
    std::string charging = "CH";
    std::string folder = "FD";
    std::string song = "MU";
    std::string playlist = "PL";
    std::string memo = "ME";
    std::string album = "AL";
    std::string home = "HM";
    std::string library = "LB";
    std::string now_playing = "NP";
    std::string settings = "ST";
    std::string about = "AB";
    std::string marginalia = "MN";
};

struct ThemeAnimations {
    int boot_fade_ms = 600;
    int popup_fade_ms = 180;
    int menu_slide_ms = 240;
    int selection_highlight_ms = 120;
    int loading_ms = 900;
    int progress_ms = 120;
    int volume_ms = 160;
    int battery_ms = 600;
    int bluetooth_search_ms = 700;
    int app_sync_ms = 900;
    int target_fps = 30;
    int max_animated_elements = 6;
};

struct ThemeResources {
    std::string root;
    std::string fallback_icon = "default";
    std::string fallback_font = "default_pixel";
    std::vector<std::string> loaded_assets;
};

struct ThemeConfig {
    std::string api_version = "ThemeAPI v1";
    bool valid = true;
    std::string source = "built-in";
};

struct ThemeManifest {
    int theme_version = 1;
    int sdk_version = 1;
    std::string author = "SHAeR";
    std::string description;
    std::string layout_profile = "240x320-safe-grid";
    std::string font_family = "default_pixel";
    std::string icon_pack = "semantic-default";
    std::string animation_profile = "battery-aware";
    std::vector<std::string> supported_features;
    std::string preview_image;
    std::vector<std::string> mandatory_screens;
};

struct ThemeDefinition {
    std::string id = "default";
    std::string display_name = "SHAeR Default";
    std::string boot_emotion = "anticipation";
    std::string now_playing_emotion = "calm immersion";
    std::string error_emotion = "playful, never alarming";
    std::string layout_signature = "token-driven default layout";
    std::string transition_signature = "token-driven fade";
    std::string animation_signature = "minimal battery-aware motion";
    ThemePalette palette;
    ThemeTypography typography;
    ThemeSpacing spacing;
    ThemeIcons icons;
    ThemeAnimations animations;
    ThemeResources resources;
    ThemeConfig config;
    ThemeManifest manifest;
};

struct ThemeRenderProfile {
    std::string id;
    std::string display_name;
    std::string layout_signature;
    std::string transition_signature;
    std::string animation_signature;
    std::string emotional_tone;
    ThemeDefinition definition;
};

struct ScreenBlueprint {
    std::string chrome;
    std::string primary_region;
    std::string selector;
    std::string transition_in;
    std::vector<std::string> lines;
};

struct ScreenRegistration {
    Screen id = Screen::Boot;
    std::string display_name;
    std::string layout_profile = "240x320-safe-grid";
    std::vector<std::string> widgets;
    std::vector<Screen> navigation_targets;
};

struct ThemeValidationResult {
    bool valid = false;
    std::vector<std::string> errors;
};

struct RenderModel {
    std::string os_name = u8"आदि Vasi OS";
    FirmwareState firmware_state = FirmwareState::Booting;
    Screen screen = Screen::Boot;
    ThemeRenderProfile theme;
    ScreenBlueprint blueprint;
    AnimationPolicy animation;
    TransitionPlan transition;
    int tick_count = 0;
    int battery_percent = 100;
    bool charging = false;
    bool wifi_connected = true;
    bool bluetooth_connected = false;
    PlaybackSnapshot playback;
    LibraryIndex library_index;
    std::vector<Track> local_library;
    int selected_index = 0;
    int scroll_offset = 0;
    std::string connection_hint;
    ConnectionState connection_state = ConnectionState::Ready;
    AudioSettings audio_settings;
    PowerProfile power;
    ClockSnapshot clock;
    SettingsUiState settings_ui;
    DeviceInfoSnapshot device_info;
    int volume = 50;
    Notification popup;
    Notebook notebook;
    std::vector<std::string> console;
};

const char* to_string(Screen screen);
const char* to_string(FirmwareState state);
const char* to_string(PlaybackSource source);
const char* to_string(PlaybackState state);
const char* to_string(ConnectionState state);
const char* to_string(ReplayGainMode mode);
const char* to_string(QualityMode mode);
const char* to_string(PowerMode mode);

}  // namespace shaer
