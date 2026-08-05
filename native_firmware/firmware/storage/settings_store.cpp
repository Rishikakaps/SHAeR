#include "settings_store.hpp"
#include "settings_catalog.hpp"

#include <filesystem>
#include <sqlite3.h>
#include <utility>
#include <vector>

namespace shaer {

namespace {

constexpr int kCurrentSchemaVersion = 5;

}

SettingsStore::SettingsStore(std::string database_path)
    : database_path_(std::move(database_path)) {}

SettingsStore::~SettingsStore() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool SettingsStore::open() {
    if (!prepare_parent_directory()) {
        return false;
    }
    if (sqlite3_open(database_path_.c_str(), &db_) != SQLITE_OK) {
        last_error_ = db_ ? sqlite3_errmsg(db_) : "sqlite open failed";
        return false;
    }
    sqlite3_busy_timeout(db_, 2500);
    return exec("PRAGMA journal_mode=WAL;") && exec("PRAGMA foreign_keys=ON;");
}

bool SettingsStore::migrate() {
    if (!exec(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "version INTEGER PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "applied_at INTEGER NOT NULL DEFAULT (unixepoch())"
        ");"
        "CREATE TABLE IF NOT EXISTS schema_meta ("
        "key TEXT PRIMARY KEY,"
        "value TEXT NOT NULL"
        ");")) {
        return false;
    }

    const int before = schema_version();
    if (before > kCurrentSchemaVersion) {
        last_error_ = "settings database schema is newer than this firmware";
        return false;
    }
    if (before < kCurrentSchemaVersion && !backup_for_rollback()) {
        return false;
    }
    if (before < 1 && !migrate_to_v1()) return false;
    if (before < 2 && !migrate_to_v2()) return false;
    if (before < 3 && !migrate_to_v3()) return false;
    if (before < 4 && !migrate_to_v4()) return false;
    if (before < 5 && !migrate_to_v5()) return false;
    return schema_version() == kCurrentSchemaVersion;
}

int SettingsStore::schema_version() const {
    if (!db_) return 0;
    const char* sql = "SELECT value FROM schema_meta WHERE key = 'schema_version';";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return 0;
    }
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    int version = 0;
    if (text) {
        try {
            version = std::stoi(reinterpret_cast<const char*>(text));
        } catch (...) {
            version = 0;
        }
    }
    sqlite3_finalize(stmt);
    return version;
}

std::string SettingsStore::rollback_path() const {
    return database_path_ + ".rollback";
}

bool SettingsStore::migrate_to_v1() {
    return exec(
        "CREATE TABLE IF NOT EXISTS settings ("
        "key TEXT PRIMARY KEY,"
        "value TEXT NOT NULL,"
        "updated_at INTEGER NOT NULL DEFAULT (unixepoch())"
        ");"
        "INSERT OR REPLACE INTO schema_migrations(version, name) VALUES(1, 'create_settings');"
        "INSERT OR REPLACE INTO schema_meta(key, value) VALUES('schema_version', '1');");
}

bool SettingsStore::migrate_to_v2() {
    return exec(
        "CREATE TABLE IF NOT EXISTS boot_state ("
        "key TEXT PRIMARY KEY,"
        "value TEXT NOT NULL,"
        "updated_at INTEGER NOT NULL DEFAULT (unixepoch())"
        ");"
        "CREATE TABLE IF NOT EXISTS setting_history ("
        "id INTEGER PRIMARY KEY,"
        "key TEXT NOT NULL,"
        "old_value TEXT,"
        "new_value TEXT NOT NULL,"
        "changed_at INTEGER NOT NULL DEFAULT (unixepoch())"
        ");"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('library.music_directory', '/var/lib/shaer/music');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('os.name', 'आदि Vasi OS');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('theme.active', 'archive_dark');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('appearance.theme', 'archive_dark');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('audio.volume', '50');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('audio.crossfade_seconds', '0');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('audio.replaygain', 'off');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('audio.quality', 'balanced');"
        "INSERT OR IGNORE INTO settings(key, value) VALUES('power.mode', 'normal');"
        "INSERT OR REPLACE INTO schema_migrations(version, name) VALUES(2, 'boot_state_and_defaults');"
        "INSERT OR REPLACE INTO schema_meta(key, value) VALUES('schema_version', '2');");
}

bool SettingsStore::migrate_to_v3() {
    for (const auto& pair : default_settings_values()) {
        const std::string sql =
            "INSERT OR IGNORE INTO settings(key, value) VALUES('" + pair.first + "', '" + pair.second + "');";
        if (!exec(sql)) {
            return false;
        }
    }
    return exec(
        "INSERT OR REPLACE INTO schema_migrations(version, name) VALUES(3, 'settings_catalog_defaults');"
        "INSERT OR REPLACE INTO schema_meta(key, value) VALUES('schema_version', '3');");
}

bool SettingsStore::migrate_to_v4() {
    return exec(
        "UPDATE settings SET value='archive_dark' WHERE key IN ('theme.active', 'appearance.theme') AND value='default';"
        "INSERT OR REPLACE INTO schema_migrations(version, name) VALUES(4, 'archive_dark_reference_theme');"
        "INSERT OR REPLACE INTO schema_meta(key, value) VALUES('schema_version', '4');");
}

bool SettingsStore::migrate_to_v5() {
    return exec(
        "INSERT OR IGNORE INTO settings(key, value) "
        "SELECT 'appearance.theme', COALESCE((SELECT value FROM settings WHERE key='theme.active'), 'archive_dark');"
        "UPDATE settings SET value='archive_dark' WHERE key IN ('theme.active', 'appearance.theme') AND value='default';"
        "INSERT OR REPLACE INTO schema_migrations(version, name) VALUES(5, 'backfill_appearance_theme');"
        "INSERT OR REPLACE INTO schema_meta(key, value) VALUES('schema_version', '5');");
}

bool SettingsStore::put(const std::string& key, const std::string& value) {
    const char* sql =
        "INSERT INTO settings(key, value, updated_at) VALUES(?, ?, unixepoch()) "
        "ON CONFLICT(key) DO UPDATE SET value=excluded.value, updated_at=unixepoch();";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    const auto old_value = get(key);
    const int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        last_error_ = sqlite3_errmsg(db_);
        return false;
    }
    const char* history_sql =
        "INSERT INTO setting_history(key, old_value, new_value, changed_at) VALUES(?, ?, ?, unixepoch());";
    sqlite3_stmt* history = nullptr;
    if (sqlite3_prepare_v2(db_, history_sql, -1, &history, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(history, 1, key.c_str(), -1, SQLITE_TRANSIENT);
        if (old_value) {
            sqlite3_bind_text(history, 2, old_value->c_str(), -1, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_null(history, 2);
        }
        sqlite3_bind_text(history, 3, value.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(history);
        sqlite3_finalize(history);
    }
    return true;
}

std::optional<std::string> SettingsStore::get(const std::string& key) const {
    const char* sql = "SELECT value FROM settings WHERE key = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    std::string value = text ? reinterpret_cast<const char*>(text) : "";
    sqlite3_finalize(stmt);
    return value;
}

std::map<std::string, std::string> SettingsStore::all_settings() const {
    std::map<std::string, std::string> values;
    const char* sql = "SELECT key, value FROM settings ORDER BY key;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        last_error_ = sqlite3_errmsg(db_);
        return values;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* key = sqlite3_column_text(stmt, 0);
        const unsigned char* value = sqlite3_column_text(stmt, 1);
        if (key) {
            values[reinterpret_cast<const char*>(key)] = value ? reinterpret_cast<const char*>(value) : "";
        }
    }
    sqlite3_finalize(stmt);
    return values;
}

RuntimeSettings SettingsStore::load_runtime_settings() {
    RuntimeSettings settings;
    auto apply_string = [this](const std::string& key, std::string* target) {
        if (auto value = get(key)) {
            *target = *value;
        }
    };
    auto apply_int = [this](const std::string& key, int* target) {
        if (auto value = get(key)) {
            try {
                *target = std::stoi(*value);
            } catch (...) {
                last_error_ = "invalid integer setting: " + key;
            }
        }
    };

    apply_string("theme.active", &settings.active_theme);
    apply_string("appearance.theme", &settings.active_theme);
    apply_string("os.name", &settings.os_name);
    apply_string("library.music_directory", &settings.music_directory);
    apply_int("audio.volume", &settings.volume);
    apply_int("audio.crossfade_seconds", &settings.crossfade_seconds);
    apply_string("audio.replaygain", &settings.replaygain_mode);
    apply_string("audio.quality", &settings.quality_mode);
    apply_string("power.mode", &settings.power_mode);
    settings.values = all_settings();
    return settings;
}

bool SettingsStore::save_runtime_settings(const RuntimeSettings& settings) {
    return put("theme.active", settings.active_theme) &&
           put("appearance.theme", settings.active_theme) &&
           put("os.name", settings.os_name) &&
           put("library.music_directory", settings.music_directory) &&
           put("audio.volume", std::to_string(settings.volume)) &&
           put("audio.crossfade_seconds", std::to_string(settings.crossfade_seconds)) &&
           put("audio.replaygain", settings.replaygain_mode) &&
           put("audio.quality", settings.quality_mode) &&
           put("power.mode", settings.power_mode);
}

std::string SettingsStore::last_error() const {
    return last_error_;
}

std::string SettingsStore::path() const {
    return database_path_;
}

bool SettingsStore::exec(const std::string& sql) const {
    char* error = nullptr;
    const int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &error);
    if (rc != SQLITE_OK) {
        last_error_ = error ? error : sqlite3_errmsg(db_);
        sqlite3_free(error);
        return false;
    }
    return true;
}

bool SettingsStore::prepare_parent_directory() const {
    const std::filesystem::path path(database_path_);
    const auto parent = path.parent_path();
    if (parent.empty()) {
        return true;
    }
    std::error_code ec;
    std::filesystem::create_directories(parent, ec);
    if (ec) {
        last_error_ = ec.message();
        return false;
    }
    return true;
}

bool SettingsStore::backup_for_rollback() const {
    if (!std::filesystem::exists(database_path_)) {
        return true;
    }
    std::error_code ec;
    std::filesystem::copy_file(
        database_path_,
        rollback_path(),
        std::filesystem::copy_options::overwrite_existing,
        ec);
    if (ec) {
        last_error_ = "could not create settings rollback: " + ec.message();
        return false;
    }
    return true;
}

}  // namespace shaer
