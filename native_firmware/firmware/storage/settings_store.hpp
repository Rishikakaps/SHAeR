#pragma once

#include "types.hpp"

#include <map>
#include <optional>
#include <string>

struct sqlite3;

namespace shaer {

class SettingsStore {
public:
    explicit SettingsStore(std::string database_path);
    ~SettingsStore();

    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;

    bool open();
    bool migrate();
    int schema_version() const;
    std::string rollback_path() const;
    bool put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    std::map<std::string, std::string> all_settings() const;
    RuntimeSettings load_runtime_settings();
    bool save_runtime_settings(const RuntimeSettings& settings);
    std::string last_error() const;
    std::string path() const;

private:
    bool backup_for_rollback() const;
    bool migrate_to_v1();
    bool migrate_to_v2();
    bool migrate_to_v3();
    bool migrate_to_v4();
    bool migrate_to_v5();
    bool exec(const std::string& sql) const;
    bool prepare_parent_directory() const;
    std::string database_path_;
    sqlite3* db_ = nullptr;
    mutable std::string last_error_;
};

}  // namespace shaer
