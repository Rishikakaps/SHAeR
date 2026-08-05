#include "settings_catalog.hpp"

#include <algorithm>

namespace shaer {

namespace {

std::string value_or(const RuntimeSettings& settings, const std::string& key, const std::string& fallback) {
    auto it = settings.values.find(key);
    return it == settings.values.end() ? fallback : it->second;
}

SettingsItem item(const RuntimeSettings& settings, std::string key, std::string label, std::string fallback, bool editable = true, bool placeholder = false) {
    SettingsItem row;
    row.key = std::move(key);
    row.label = std::move(label);
    row.value = value_or(settings, row.key, std::move(fallback));
    row.editable = editable;
    row.placeholder = placeholder;
    return row;
}

SettingsItem readonly(std::string key, std::string label, std::string value) {
    SettingsItem row;
    row.key = std::move(key);
    row.label = std::move(label);
    row.value = std::move(value);
    row.editable = false;
    return row;
}

SettingsCategory category(std::string id, std::string title, std::vector<SettingsItem> items) {
    return {std::move(id), std::move(title), std::move(items)};
}

}  // namespace

std::map<std::string, std::string> default_settings_values() {
    return {
        {"appearance.brightness", "80"},
        {"appearance.theme", "archive_dark"},
        {"appearance.screen_timeout", "30s"},
        {"appearance.animation_speed", "normal"},
        {"appearance.boot_animation", "on"},
        {"appearance.font_size", "normal"},
        {"audio.volume", "50"},
        {"audio.balance", "center"},
        {"audio.mono", "off"},
        {"audio.crossfade_seconds", "0"},
        {"audio.gapless", "on"},
        {"audio.replaygain", "off"},
        {"audio.equalizer", "placeholder"},
        {"playback.resume", "off"},
        {"playback.remember_queue", "on"},
        {"playback.shuffle_default", "off"},
        {"playback.repeat_default", "off"},
        {"playback.auto_play", "off"},
        {"playback.recently_played", "keep"},
        {"connectivity.wifi_reconnect", "ready"},
        {"connectivity.hostname", "shaer"},
        {"connectivity.spotify", "placeholder"},
        {"connectivity.companion", "placeholder"},
        {"storage.manual_rescan", "run"},
        {"storage.rebuild_database", "run"},
        {"power.sleep_timer", "45m"},
        {"power.auto_sleep", "on"},
        {"power.auto_shutdown", "45m"},
        {"power.shutdown", "confirm"},
        {"power.restart", "confirm"},
        {"privacy.clear_recently_played", "run"},
        {"privacy.clear_cache", "run"},
        {"privacy.factory_reset", "placeholder"},
        {"advanced.developer_mode", "off"},
        {"advanced.recovery_mode", "available"},
    };
}

std::vector<SettingsCategory> build_settings_catalog(
    const RuntimeSettings& settings,
    const DeviceInfoSnapshot& device,
    int battery_percent,
    bool charging,
    const LibraryIndex& library) {
    return {
        category("appearance", "Appearance", {
            item(settings, "appearance.theme", "Active Theme", settings.active_theme),
            item(settings, "appearance.brightness", "Brightness", "80"),
            item(settings, "appearance.screen_timeout", "Screen Timeout", "30s"),
            item(settings, "appearance.animation_speed", "Animation Speed", "normal"),
            item(settings, "appearance.boot_animation", "Boot Animation", "on"),
            item(settings, "appearance.font_size", "Font Size", "normal"),
        }),
        category("audio", "Audio", {
            item(settings, "audio.volume", "Volume", std::to_string(settings.volume)),
            item(settings, "audio.balance", "Balance", "center"),
            item(settings, "audio.mono", "Mono Mode", "off"),
            item(settings, "audio.crossfade_seconds", "Crossfade", std::to_string(settings.crossfade_seconds)),
            item(settings, "audio.gapless", "Gapless Playback", "on"),
            item(settings, "audio.replaygain", "ReplayGain", settings.replaygain_mode, true, true),
            item(settings, "audio.equalizer", "Equalizer", "placeholder", true, true),
        }),
        category("playback", "Playback", {
            item(settings, "playback.resume", "Resume Playback", "off"),
            item(settings, "playback.remember_queue", "Remember Queue", "on"),
            item(settings, "playback.shuffle_default", "Shuffle Default", "off"),
            item(settings, "playback.repeat_default", "Repeat Default", "off"),
            item(settings, "playback.auto_play", "Auto Play", "off"),
            item(settings, "playback.recently_played", "Recently Played", "keep"),
        }),
        category("connectivity", "Connectivity", {
            readonly("connectivity.wifi_status", "Wi-Fi Status", device.ip_address == "unknown" ? "unknown" : "connected"),
            item(settings, "connectivity.wifi_reconnect", "Wi-Fi Reconnect", "ready"),
            readonly("connectivity.hostname_live", "Hostname", device.hostname),
            readonly("connectivity.ip_address", "IP Address", device.ip_address),
            item(settings, "connectivity.spotify", "Spotify", "placeholder", true, true),
            item(settings, "connectivity.companion", "Companion App", "placeholder", true, true),
        }),
        category("storage", "Storage", {
            readonly("storage.music_statistics", "Music Statistics", std::to_string(library.songs.size()) + " songs"),
            readonly("storage.free", "Free Storage", std::to_string(device.storage_free_mb) + "MB"),
            readonly("storage.used", "Used Storage", std::to_string(device.storage_used_mb) + "MB"),
            readonly("storage.sd_status", "SD Card Status", "mounted"),
            item(settings, "storage.manual_rescan", "Manual Library Rescan", "run"),
            item(settings, "storage.rebuild_database", "Library DB Rebuild", "run"),
        }),
        category("power", "Power", {
            readonly("power.battery_percent", "Battery", std::to_string(battery_percent) + "%"),
            readonly("power.charging", "Charging", charging ? "yes" : "no"),
            item(settings, "power.sleep_timer", "Sleep Timer", "45m"),
            item(settings, "power.auto_sleep", "Auto Sleep", "on"),
            item(settings, "power.auto_shutdown", "Auto Shutdown", "45m"),
            readonly("power.cpu_temp", "CPU Temperature", std::to_string(device.cpu_temp_c) + "C"),
            readonly("power.diagnostics", "Power Diagnostics", "available"),
            item(settings, "power.shutdown", "Shutdown", "confirm"),
            item(settings, "power.restart", "Restart", "confirm"),
        }),
        category("device", "Device", {
            readonly("device.display_test", "Display Test", "available"),
            readonly("device.encoder_test", "Encoder Test", "available"),
            readonly("device.dac_test", "DAC Test", "available"),
            readonly("device.sdcard_test", "SD Card Test", "available"),
            readonly("device.gpio_test", "GPIO Test", "available"),
            readonly("device.firmware_version", "Firmware Version", device.firmware_version),
            readonly("device.hardware_revision", "Hardware Revision", device.hardware_revision),
        }),
        category("privacy", "Privacy", {
            item(settings, "privacy.clear_recently_played", "Clear Recently Played", "run"),
            item(settings, "privacy.clear_cache", "Clear Cache", "run"),
            item(settings, "privacy.factory_reset", "Factory Reset", "placeholder", true, true),
        }),
        category("advanced", "Advanced", {
            item(settings, "advanced.developer_mode", "Developer Mode", "off"),
            readonly("advanced.logs", "Logs", value_or(settings, "advanced.developer_mode", "off") == "on" ? "available" : "locked"),
            readonly("advanced.fps", "FPS", std::to_string(device.fps)),
            readonly("advanced.cpu_usage", "CPU Usage", std::to_string(device.cpu_usage_percent) + "%"),
            readonly("advanced.memory", "Memory Usage", std::to_string(device.memory_used_mb) + "MB"),
            readonly("advanced.thread_monitor", "Thread Monitor", "available"),
            readonly("advanced.gpio_monitor", "GPIO Monitor", "available"),
            readonly("advanced.diagnostics", "Diagnostics", "available"),
            item(settings, "advanced.recovery_mode", "Recovery Mode", "available"),
        }),
        category("about", "About", {
            readonly("about.firmware_version", "Firmware Version", device.firmware_version),
            readonly("about.build_number", "Build Number", device.build_number),
            readonly("about.git_commit", "Git Commit", device.git_commit),
            readonly("about.git_tag", "Git Tag", device.git_tag),
            readonly("about.hardware_revision", "Hardware Revision", device.hardware_revision),
            readonly("about.device_name", "Device Name", device.device_name),
            readonly("about.storage", "Storage", std::to_string(device.storage_free_mb) + "MB free"),
            readonly("about.battery_health", "Battery Health", std::to_string(device.battery_health_percent) + "%"),
            readonly("about.credits", "Credits", "Rishika + SHAeR"),
        }),
    };
}

std::string next_setting_value(const std::string& key, const std::string& current) {
    auto cycle = [&current](const std::vector<std::string>& values) {
        auto it = std::find(values.begin(), values.end(), current);
        if (it == values.end() || ++it == values.end()) return values.front();
        return *it;
    };
    if (key == "appearance.brightness") return cycle({"20", "40", "60", "80", "100"});
    if (key == "appearance.theme") return cycle({"archive_dark", "default", "bombay_ticket", "japanese_punk", "windows_xp", "ghibli_garden", "indian_raga"});
    if (key == "appearance.screen_timeout") return cycle({"15s", "30s", "60s", "never"});
    if (key == "appearance.animation_speed") return cycle({"slow", "normal", "fast", "reduced"});
    if (key == "appearance.boot_animation") return current == "on" ? "off" : "on";
    if (key == "appearance.font_size") return cycle({"small", "normal", "large"});
    if (key == "audio.volume") return cycle({"0", "25", "50", "75", "100"});
    if (key == "audio.balance") return cycle({"left", "center", "right"});
    if (key == "audio.mono") return current == "on" ? "off" : "on";
    if (key == "audio.crossfade_seconds") return cycle({"0", "1", "2", "3", "5"});
    if (key == "audio.gapless") return current == "on" ? "off" : "on";
    if (key == "audio.replaygain") return cycle({"off", "track", "album"});
    if (key == "playback.resume" || key == "playback.remember_queue" ||
        key == "playback.shuffle_default" || key == "playback.auto_play" ||
        key == "power.auto_sleep" || key == "advanced.developer_mode") {
        return current == "on" ? "off" : "on";
    }
    if (key == "playback.repeat_default") return cycle({"off", "one", "all"});
    if (key == "playback.recently_played") return cycle({"keep", "clear"});
    if (key == "power.sleep_timer") return cycle({"off", "15m", "30m", "45m", "60m", "90m", "120m"});
    if (key == "power.auto_shutdown") return cycle({"off", "30m", "45m", "60m"});
    if (key == "storage.manual_rescan" || key == "storage.rebuild_database" ||
        key == "privacy.clear_recently_played" || key == "privacy.clear_cache") {
        return "queued";
    }
    return current;
}

bool setting_is_action(const std::string& key) {
    return key == "storage.manual_rescan" ||
           key == "storage.rebuild_database" ||
           key == "privacy.clear_recently_played" ||
           key == "privacy.clear_cache" ||
           key == "advanced.recovery_mode";
}

}  // namespace shaer
