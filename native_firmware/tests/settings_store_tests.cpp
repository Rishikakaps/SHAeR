#include "settings_store.hpp"

#include <cassert>
#include <filesystem>
#include <string>
#include <unistd.h>

int main() {
    const std::string dir = "/tmp/shaer_settings_test_" + std::to_string(getpid());
    const std::string db = dir + "/library.db";
    std::filesystem::remove_all(dir);

    shaer::SettingsStore store(db);
    assert(store.open());
    assert(store.migrate());
    assert(store.schema_version() == 5);
    assert(store.rollback_path() == db + ".rollback");
    assert(store.get("theme.active").value() == "archive_dark");
    assert(store.get("appearance.theme").value() == "archive_dark");
    assert(store.put("theme.active", "indian_raga"));
    assert(store.get("theme.active").value() == "indian_raga");

    shaer::RuntimeSettings settings;
    settings.active_theme = "windows_xp";
    settings.volume = 72;
    settings.crossfade_seconds = 6;
    settings.replaygain_mode = "album";
    settings.quality_mode = "archive_quality";
    settings.power_mode = "battery_saver";
    assert(store.save_runtime_settings(settings));

    const auto loaded = store.load_runtime_settings();
    assert(loaded.active_theme == "windows_xp");
    assert(loaded.volume == 72);
    assert(loaded.crossfade_seconds == 6);
    assert(loaded.replaygain_mode == "album");
    assert(loaded.quality_mode == "archive_quality");
    assert(loaded.power_mode == "battery_saver");
    assert(loaded.values.at("appearance.brightness") == "80");
    assert(loaded.values.at("power.sleep_timer") == "45m");

    std::filesystem::remove_all(dir);
    return 0;
}
