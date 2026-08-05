#pragma once

#include "hal.hpp"
#include "boot_recovery.hpp"
#include "settings_store.hpp"
#include "structured_logger.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace shaer {

enum class BootStepStatus {
    Ok,
    Warning,
    Failed,
};

struct BootStepResult {
    std::string name;
    BootStepStatus status = BootStepStatus::Ok;
    int elapsed_ms = 0;
    std::string message;
    std::string error_code = "OK";
};

struct BootReport {
    bool usable = true;
    bool safe_mode = false;
    int boot_counter = 0;
    int crash_counter = 0;
    int total_ms = 0;
    RuntimeSettings settings;
    std::vector<BootStepResult> steps;

    bool has_failures() const;
    std::string diagnostic_title() const;
    std::string diagnostic_body() const;
};

struct BootConfig {
    std::string data_directory = "data";
    std::string log_directory = "logs";
    int boot_target_ms = 8000;
    bool enable_watchdog = true;
};

class HardwareDiagnostics {
public:
    virtual ~HardwareDiagnostics() = default;
    virtual BootStepResult initialize_gpio() = 0;
    virtual BootStepResult initialize_spi() = 0;
    virtual BootStepResult initialize_i2c() = 0;
    virtual BootStepResult initialize_alsa() = 0;
    virtual BootStepResult initialize_bluetooth() = 0;
    virtual BootStepResult initialize_wifi() = 0;
    virtual BootStepResult initialize_battery_gauge() = 0;
    virtual BootStepResult initialize_display() = 0;
};

class Watchdog {
public:
    virtual ~Watchdog() = default;
    virtual BootStepResult arm() = 0;
    virtual void kick() = 0;
    virtual void disarm() = 0;
};

class DisabledWatchdog final : public Watchdog {
public:
    BootStepResult arm() override;
    void kick() override;
    void disarm() override;
};

class BootFirmware {
public:
    BootFirmware(
        BootConfig config,
        HardwareDiagnostics& diagnostics,
        SettingsStore& settings_store,
        StructuredLogger& logger,
        Watchdog& watchdog,
        Display* display);

    BootReport cold_boot();
    void mark_boot_successful();
    void clean_shutdown(const std::string& reason);

private:
    BootStepResult timed_step(const std::string& name, BootStepResult (*fn)(HardwareDiagnostics&));
    void record_step(BootReport* report, BootStepResult step);
    void render_boot_frame(const std::string& message, int progress_percent);
    void log_step(const BootStepResult& step);

    BootConfig config_;
    HardwareDiagnostics& diagnostics_;
    SettingsStore& settings_store_;
    StructuredLogger& logger_;
    Watchdog& watchdog_;
    Display* display_;
    std::chrono::steady_clock::time_point boot_started_;
};

}  // namespace shaer
