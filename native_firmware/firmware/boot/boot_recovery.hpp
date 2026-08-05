#pragma once

#include "settings_store.hpp"

#include <string>

namespace shaer {

struct BootRecoveryStatus {
    bool previous_boot_successful = true;
    bool safe_mode = false;
    int boot_counter = 0;
    int crash_counter = 0;
    std::string reason = "normal";
};

class BootRecoveryManager {
public:
    explicit BootRecoveryManager(SettingsStore& settings);

    BootRecoveryStatus begin_boot();
    bool mark_boot_successful();

private:
    int int_value(const std::string& key, int fallback) const;
    bool put_int(const std::string& key, int value);
    int now_seconds() const;

    SettingsStore& settings_;
};

}  // namespace shaer
