#include "boot_recovery.hpp"

#include <ctime>

namespace shaer {

namespace {

constexpr int kCrashWindowSeconds = 30;
constexpr int kSafeModeCrashThreshold = 3;

}

BootRecoveryManager::BootRecoveryManager(SettingsStore& settings) : settings_(settings) {}

BootRecoveryStatus BootRecoveryManager::begin_boot() {
    BootRecoveryStatus status;
    const int now = now_seconds();
    const int in_progress = int_value("boot.in_progress", 0);
    const int last_start = int_value("boot.last_start_epoch", 0);
    const int first_crash = int_value("boot.first_crash_epoch", 0);
    int crash_counter = int_value("boot.crash_counter", 0);

    status.previous_boot_successful = in_progress == 0;
    if (in_progress != 0 && now - last_start <= kCrashWindowSeconds) {
        const bool same_window = first_crash > 0 && now - first_crash <= kCrashWindowSeconds;
        crash_counter = same_window ? crash_counter + 1 : 1;
        put_int("boot.first_crash_epoch", same_window ? first_crash : now);
        status.reason = "previous boot did not complete";
    } else if (in_progress != 0) {
        crash_counter = 1;
        put_int("boot.first_crash_epoch", now);
        status.reason = "previous boot incomplete outside crash window";
    }

    const int boot_counter = int_value("boot.counter", 0) + 1;
    put_int("boot.counter", boot_counter);
    put_int("boot.crash_counter", crash_counter);
    put_int("boot.last_start_epoch", now);
    put_int("boot.in_progress", 1);

    status.boot_counter = boot_counter;
    status.crash_counter = crash_counter;
    status.safe_mode = crash_counter >= kSafeModeCrashThreshold;
    settings_.put("boot.safe_mode", status.safe_mode ? "1" : "0");
    if (status.safe_mode) {
        status.reason = "crash loop detected";
    }
    return status;
}

bool BootRecoveryManager::mark_boot_successful() {
    return settings_.put("boot.in_progress", "0") &&
           settings_.put("boot.last_successful_epoch", std::to_string(now_seconds())) &&
           settings_.put("boot.crash_counter", "0") &&
           settings_.put("boot.safe_mode", "0");
}

int BootRecoveryManager::int_value(const std::string& key, int fallback) const {
    auto value = settings_.get(key);
    if (!value) return fallback;
    try {
        return std::stoi(*value);
    } catch (...) {
        return fallback;
    }
}

bool BootRecoveryManager::put_int(const std::string& key, int value) {
    return settings_.put(key, std::to_string(value));
}

int BootRecoveryManager::now_seconds() const {
    return static_cast<int>(std::time(nullptr));
}

}  // namespace shaer
