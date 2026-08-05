#include "theme_engine.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace shaer {

namespace {

ThemeColor parse_color(const std::string& value, ThemeColor fallback) {
    if (value.size() != 7 || value[0] != '#') return fallback;
    try {
        const auto byte = [&](size_t offset) -> unsigned char {
            return static_cast<unsigned char>(std::stoi(value.substr(offset, 2), nullptr, 16));
        };
        return {byte(1), byte(3), byte(5)};
    } catch (...) {
        return fallback;
    }
}

int parse_int(const std::map<std::string, std::string>& fields, const std::string& key, int fallback) {
    auto it = fields.find(key);
    if (it == fields.end()) return fallback;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return fallback;
    }
}

std::string parse_string(const std::map<std::string, std::string>& fields, const std::string& key, std::string fallback) {
    auto it = fields.find(key);
    return it == fields.end() || it->second.empty() ? std::move(fallback) : it->second;
}

ThemeDefinition default_theme() {
    ThemeDefinition theme;
    theme.id = "default";
    theme.display_name = "SHAeR Default";
    theme.boot_emotion = "anticipation and curiosity";
    theme.now_playing_emotion = "calm immersion";
    theme.error_emotion = "playful, never alarming";
    theme.layout_signature = "common embedded OS skeleton";
    theme.transition_signature = "quiet token fade";
    theme.animation_signature = "minimal battery-aware motion";
    theme.resources.root = "assets/themes/default";
    theme.config.source = "built-in fallback";
    theme.manifest.description = "SHAeR engineering reference theme";
    theme.manifest.layout_profile = "240x320-safe-grid";
    theme.manifest.font_family = theme.typography.primary;
    theme.manifest.icon_pack = "semantic-default";
    theme.manifest.animation_profile = theme.animation_signature;
    theme.manifest.supported_features = {"playback", "local-library", "spotify", "voice-memo", "marginalia", "settings"};
    theme.manifest.preview_image = "reference_sheets/archive_dark.png";
    theme.manifest.mandatory_screens = {"Boot", "Logo", "Loading", "Home", "Library", "NowPlaying", "VoiceMemo", "Recorder", "Marginalia", "Settings", "Bluetooth", "SpotifyConnect", "Sync", "Storage", "About", "Error", "Sleep"};
    return theme;
}

ThemeDefinition named_minimal_theme(std::string id, std::string display_name) {
    ThemeDefinition theme = default_theme();
    theme.id = std::move(id);
    theme.display_name = std::move(display_name);
    theme.resources.root = "assets/themes/" + theme.id;
    theme.config.source = "registered minimal";
    theme.manifest.description = theme.display_name + " visual world";
    theme.manifest.preview_image = "reference_sheets/" + theme.id + ".png";
    return theme;
}

ThemeDefinition archive_dark_theme() {
    ThemeDefinition theme = default_theme();
    theme.id = "archive_dark";
    theme.display_name = "Archive Dark";
    theme.boot_emotion = "anticipation and curiosity";
    theme.now_playing_emotion = "calm immersion";
    theme.error_emotion = "playful, never alarming";
    theme.layout_signature = "Figma-faithful archive terminal frame";
    theme.transition_signature = "phosphor fade with scanline settle";
    theme.animation_signature = "cursor blink and low-glow meters";
    theme.palette.background = {0, 12, 14};
    theme.palette.foreground = {232, 236, 232};
    theme.palette.accent = {0, 232, 139};
    theme.palette.selection = {0, 232, 139};
    theme.palette.disabled = {128, 142, 140};
    theme.palette.popup = {0, 6, 8};
    theme.palette.border = {86, 90, 88};
    theme.palette.status_bar = {64, 64, 64};
    theme.palette.footer = {64, 64, 64};
    theme.palette.progress_background = {52, 52, 52};
    theme.palette.progress_foreground = {232, 236, 232};
    theme.palette.volume_bar = {0, 232, 139};
    theme.palette.warning = {230, 120, 45};
    theme.typography.primary = "Copperplate";
    theme.typography.secondary = "Copperplate";
    theme.typography.heading_size = 1;
    theme.typography.body_size = 1;
    theme.spacing.status_height = 10;
    theme.spacing.footer_height = 14;
    theme.spacing.row_height = 16;
    theme.spacing.popup_width = 184;
    theme.spacing.popup_height = 112;
    theme.resources.root = "assets/themes/archive_dark";
    theme.config.source = "registered archive dark reference";
    theme.manifest.font_family = theme.typography.primary;
    theme.manifest.preview_image = "reference_sheets/archive_dark.png";
    return theme;
}

std::map<std::string, std::string> read_properties(const std::filesystem::path& path) {
    std::map<std::string, std::string> fields;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        fields[line.substr(0, eq)] = line.substr(eq + 1);
    }
    return fields;
}

}  // namespace

ThemeAssets::ThemeAssets(std::string fallback_root) : fallback_root_(std::move(fallback_root)) {}

ThemeResources ThemeAssets::load_active(const ThemeDefinition& definition) const {
    ThemeResources resources = definition.resources;
    namespace fs = std::filesystem;
    const fs::path root = resources.root.empty() ? fs::path(fallback_root_) : fs::path(resources.root);
    resources.root = root.string();
    resources.loaded_assets.clear();

    std::error_code ec;
    if (fs::exists(root, ec)) {
        for (const auto& entry : fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (entry.is_regular_file(ec)) {
                resources.loaded_assets.push_back(entry.path().string());
            }
        }
    }

    if (resources.loaded_assets.empty() && root.string() != fallback_root_) {
        resources.root = fallback_root_;
    }
    return resources;
}

std::string ThemeAssets::resolve_asset(const ThemeResources& resources, const std::string& relative_path) const {
    namespace fs = std::filesystem;
    const fs::path active = fs::path(resources.root) / relative_path;
    std::error_code ec;
    if (fs::exists(active, ec)) {
        return active.string();
    }
    return (fs::path(fallback_root_) / relative_path).string();
}

ThemeRegistry::ThemeRegistry() {
    register_theme(default_theme());
    register_theme(archive_dark_theme());
    register_theme(named_minimal_theme("bombay_ticket", "Bombay Ticket"));
    register_theme(named_minimal_theme("indian_raga", "Indian Raga"));
    register_theme(named_minimal_theme("windows_xp", "Windows XP"));
    register_theme(named_minimal_theme("japanese_punk", "Japanese Punk"));
    register_theme(named_minimal_theme("ghibli_garden", "Ghibli Garden"));
}

bool ThemeRegistry::register_theme(ThemeDefinition definition) {
    if (definition.id.empty()) {
        definition.id = "default";
    }
    if (!validate(definition).valid) return false;
    definitions_[definition.id] = std::move(definition);
    return true;
}

ThemeValidationResult ThemeRegistry::validate(const ThemeDefinition& definition) const {
    ThemeValidationResult result;
    const auto require = [&](bool condition, const std::string& field) {
        if (!condition) result.errors.push_back("missing manifest field: " + field);
    };
    require(!definition.id.empty(), "theme_id");
    require(!definition.display_name.empty(), "theme_name");
    require(definition.manifest.theme_version > 0, "theme_version");
    require(definition.manifest.sdk_version == 1, "sdk_version=1");
    require(!definition.manifest.author.empty(), "author");
    require(!definition.manifest.description.empty(), "description");
    require(!definition.manifest.preview_image.empty(), "preview_image");
    require(!definition.manifest.layout_profile.empty(), "layout_profile");
    require(!definition.manifest.icon_pack.empty(), "icon_pack");
    require(!definition.manifest.animation_profile.empty(), "animation_profile");
    require(!definition.manifest.mandatory_screens.empty(), "mandatory_screens");
    result.valid = result.errors.empty();
    return result;
}

ThemeDefinition ThemeRegistry::resolve(const std::string& id) const {
    auto it = definitions_.find(id);
    if (it != definitions_.end()) return it->second;
    return definitions_.at("default");
}

std::vector<std::string> ThemeRegistry::available_ids() const {
    std::vector<std::string> ids;
    ids.reserve(definitions_.size());
    for (const auto& entry : definitions_) {
        ids.push_back(entry.first);
    }
    return ids;
}

bool ThemeRegistry::contains(const std::string& id) const {
    return definitions_.find(id) != definitions_.end();
}

ThemeRenderProfile ThemeRenderer::render_profile(const ThemeDefinition& definition) const {
    return {
        definition.id,
        definition.display_name,
        definition.layout_signature,
        definition.transition_signature,
        definition.animation_signature,
        definition.now_playing_emotion,
        definition,
    };
}

ScreenBlueprint ThemeRenderer::screen_blueprint(const ThemeDefinition& definition, Screen screen) const {
    const std::string transition = definition.transition_signature.empty()
        ? "token fade"
        : definition.transition_signature;
    switch (screen) {
        case Screen::Boot:
            return {"theme loading shell", "boot content", "boot focus", transition, {"LIBRA CONSTELLATION", "LOADING D: DRIVE...", "MHM MHM"}};
        case Screen::Home:
            return {"theme status shell", "home menu", "selection token", transition, {"MUSIC", "NOW PLAYING", "SETTINGS", "ABOUT"}};
        case Screen::Library:
            return {"theme status shell", "music list", "selection token", transition, {"Songs", "Albums", "Artists", "Folders"}};
        case Screen::NowPlaying:
            return {"theme player shell", "player layout", "progress token", transition, {"Title", "Artist", "Album", "Progress"}};
        case Screen::Marginalia:
            return {"theme notebook shell", "contextual notebook", "page token", transition, {"Song", "Notebook", "Page", "Stylus"}};
        case Screen::VoiceArchive:
            return {"theme status shell", "memory list", "selection token", transition, {"Memory Mode", "Track Memories", "Album Memories"}};
        case Screen::BluetoothConnect:
            return {"theme connection shell", "bluetooth pairing", "aura token", transition, {"Bluetooth", "Spotify Connect", "Device detected"}};
        case Screen::Settings:
            return {"theme settings shell", "settings list", "selection token", transition, {"Appearance", "Audio", "Power", "Device"}};
        case Screen::About:
            return {"theme info shell", "about list", "selection token", transition, {"Firmware", "Hardware", "Credits"}};
        case Screen::Popup:
            return {"theme popup shell", "centered popup", "action token", transition, {"Title", "Message", "Primary", "Secondary"}};
        case Screen::Aod:
            return {"theme low-power shell", "clock", "ambient token", transition, {"Always On"}};
        case Screen::Charging:
            return {"theme charging shell", "battery", "charging token", transition, {"Charging"}};
        case Screen::Sleep:
            return {"theme sleep shell", "sleep", "wake token", transition, {"Sleep"}};
        case Screen::Shutdown:
            return {"theme shutdown shell", "goodbye", "shutdown token", transition, {"Goodbye"}};
    }
    return {"theme shell", "content", "selection", transition, {"SHAeR"}};
}

TransitionPlan ThemeRenderer::transition_plan(
    const ThemeDefinition& definition,
    Screen from,
    Screen to,
    bool blocks_input,
    std::string reason) const {
    const int duration = blocks_input
        ? definition.animations.popup_fade_ms
        : definition.animations.menu_slide_ms;
    return {
        from,
        to,
        definition.transition_signature.empty() ? "token fade" : definition.transition_signature,
        duration,
        blocks_input,
        false,
        std::move(reason),
    };
}

AnimationPolicy ThemeRenderer::animation_policy(const ThemeDefinition& definition) const {
    return {
        definition.animations.target_fps,
        definition.animations.max_animated_elements,
        true,
        false,
        definition.animation_signature.empty() ? "theme tokens" : definition.animation_signature,
    };
}

ThemeManager::ThemeManager() {
    active_ = load_pack_or_default("default");
    active_resources_ = assets_.load_active(active_);
}

const ThemeDefinition& ThemeManager::active_definition() const {
    return active_;
}

void ThemeManager::set_active(std::string id) {
    active_ = load_pack_or_default(id);
    active_resources_ = assets_.load_active(active_);
    active_.resources = active_resources_;
}

void ThemeManager::cycle_active() {
    const auto ids = available_theme_ids();
    auto it = std::find(ids.begin(), ids.end(), active_.id);
    if (it == ids.end() || ++it == ids.end()) {
        set_active(ids.empty() ? "default" : ids.front());
    } else {
        set_active(*it);
    }
}

std::vector<std::string> ThemeManager::available_theme_ids() const {
    return registry_.available_ids();
}

ThemeRenderProfile ThemeManager::render_profile() const {
    return renderer_.render_profile(active_);
}

ScreenBlueprint ThemeManager::screen_blueprint(Screen screen) const {
    return renderer_.screen_blueprint(active_, screen);
}

TransitionPlan ThemeManager::transition_plan(Screen from, Screen to, bool blocks_input, std::string reason) const {
    return renderer_.transition_plan(active_, from, to, blocks_input, std::move(reason));
}

AnimationPolicy ThemeManager::animation_policy() const {
    return renderer_.animation_policy(active_);
}

const ThemeResources& ThemeManager::active_resources() const {
    return active_resources_;
}

size_t ThemeManager::active_asset_count() const {
    return active_resources_.loaded_assets.size();
}

ThemeValidationResult ThemeManager::validation() const {
    return registry_.validate(active_);
}

ThemeDefinition ThemeManager::load_pack_or_default(const std::string& id) const {
    if (auto packed = load_profile_from_pack(id)) {
        return *packed;
    }
    return registry_.resolve(id);
}

std::optional<ThemeDefinition> ThemeManager::load_profile_from_pack(const std::string& id) const {
    namespace fs = std::filesystem;
    const std::vector<fs::path> roots = {
        fs::path("assets/themes"),
        fs::path("/var/lib/shaer/themes"),
    };
    for (const auto& root : roots) {
        const fs::path path = root / id / "theme.properties";
        std::error_code ec;
        if (!fs::exists(path, ec)) continue;
        const auto fields = read_properties(path);
        ThemeDefinition theme = registry_.resolve(id);
        theme.id = id;
        theme.display_name = parse_string(fields, "display_name", theme.display_name);
        theme.boot_emotion = parse_string(fields, "boot_emotion", theme.boot_emotion);
        theme.now_playing_emotion = parse_string(fields, "now_playing_emotion", theme.now_playing_emotion);
        theme.error_emotion = parse_string(fields, "error_emotion", theme.error_emotion);
        theme.layout_signature = parse_string(fields, "layout_signature", theme.layout_signature);
        theme.transition_signature = parse_string(fields, "transition_signature", theme.transition_signature);
        theme.animation_signature = parse_string(fields, "animation_signature", theme.animation_signature);
        theme.animations.target_fps = parse_int(fields, "normal_target_fps", theme.animations.target_fps);
        theme.animations.max_animated_elements = parse_int(fields, "max_rich_animated_elements", theme.animations.max_animated_elements);
        theme.palette.background = parse_color(parse_string(fields, "background", ""), theme.palette.background);
        theme.palette.foreground = parse_color(parse_string(fields, "foreground", ""), theme.palette.foreground);
        theme.palette.accent = parse_color(parse_string(fields, "accent", ""), theme.palette.accent);
        theme.palette.selection = parse_color(parse_string(fields, "selection", ""), theme.palette.selection);
        theme.palette.disabled = parse_color(parse_string(fields, "disabled", ""), theme.palette.disabled);
        theme.palette.popup = parse_color(parse_string(fields, "popup", ""), theme.palette.popup);
        theme.palette.border = parse_color(parse_string(fields, "border", ""), theme.palette.border);
        theme.palette.status_bar = parse_color(parse_string(fields, "status_bar", ""), theme.palette.status_bar);
        theme.palette.footer = parse_color(parse_string(fields, "footer", ""), theme.palette.footer);
        theme.typography.primary = parse_string(fields, "primary_font", theme.typography.primary);
        theme.typography.secondary = parse_string(fields, "secondary_font", theme.typography.secondary);
        theme.resources.root = (root / id).string();
        theme.config.source = path.string();
        return theme;
    }
    return std::nullopt;
}

const ThemeDefinition& ThemeEngine::active_theme() const {
    return manager_.active_definition();
}

const ThemeDefinition& ThemeEngine::active_definition() const {
    return manager_.active_definition();
}

ThemeRenderProfile ThemeEngine::render_profile() const {
    return manager_.render_profile();
}

ScreenBlueprint ThemeEngine::screen_blueprint(Screen screen) const {
    return manager_.screen_blueprint(screen);
}

TransitionPlan ThemeEngine::transition_plan(Screen from, Screen to, bool blocks_input, std::string reason) const {
    return manager_.transition_plan(from, to, blocks_input, std::move(reason));
}

AnimationPolicy ThemeEngine::animation_policy() const {
    return manager_.animation_policy();
}

void ThemeEngine::set_theme(std::string id) {
    manager_.set_active(std::move(id));
}

void ThemeEngine::cycle_theme() {
    manager_.cycle_active();
}

std::vector<std::string> ThemeEngine::available_theme_ids() const {
    return manager_.available_theme_ids();
}

const ThemeResources& ThemeEngine::active_resources() const {
    return manager_.active_resources();
}

size_t ThemeEngine::active_asset_count() const {
    return manager_.active_asset_count();
}

ThemeValidationResult ThemeEngine::validation() const {
    return manager_.validation();
}

}  // namespace shaer
