#include "boot_firmware.hpp"
#include "error_codes.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace shaer {

namespace {

const char* status_name(BootStepStatus status) {
    switch (status) {
        case BootStepStatus::Ok: return "ok";
        case BootStepStatus::Warning: return "warning";
        case BootStepStatus::Failed: return "failed";
    }
    return "unknown";
}

BootStepResult call_gpio(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_gpio(); }
BootStepResult call_spi(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_spi(); }
BootStepResult call_i2c(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_i2c(); }
BootStepResult call_alsa(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_alsa(); }
BootStepResult call_bluetooth(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_bluetooth(); }
BootStepResult call_wifi(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_wifi(); }
BootStepResult call_battery(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_battery_gauge(); }
BootStepResult call_display(HardwareDiagnostics& diagnostics) { return diagnostics.initialize_display(); }

std::string code_for_step(const BootStepResult& step) {
    if (step.status == BootStepStatus::Ok) return ErrorCode::Ok;
    if (!step.error_code.empty() && step.error_code != ErrorCode::Ok) return step.error_code;
    if (step.name == "GPIO") return ErrorCode::Gpio;
    if (step.name == "SPI") return ErrorCode::Spi;
    if (step.name == "I2C") return ErrorCode::I2c;
    if (step.name == "ALSA") return ErrorCode::Alsa;
    if (step.name == "Bluetooth") return ErrorCode::Bluetooth;
    if (step.name == "WiFi") return ErrorCode::Wifi;
    if (step.name == "Battery Gauge") return ErrorCode::BatteryMissing;
    if (step.name == "Display") return ErrorCode::Display;
    if (step.name == "Watchdog") return ErrorCode::Watchdog;
    if (step.name == "Settings") return ErrorCode::SettingsMigration;
    if (step.name == "Crash Recovery") return ErrorCode::CrashLoop;
    return "E999";
}

}  // namespace

bool BootReport::has_failures() const {
    return std::any_of(steps.begin(), steps.end(), [](const BootStepResult& step) {
        return step.status == BootStepStatus::Failed || step.status == BootStepStatus::Warning;
    });
}

std::string BootReport::diagnostic_title() const {
    return usable ? "Hardware needs attention" : "Boot diagnostics";
}

std::string BootReport::diagnostic_body() const {
    std::ostringstream body;
    int count = 0;
    for (const auto& step : steps) {
        if (step.status == BootStepStatus::Ok) {
            continue;
        }
        if (count > 0) {
            body << "; ";
        }
        body << step.error_code << " " << step.name << ": " << step.message;
        ++count;
    }
    if (count == 0) {
        body << "All startup checks passed.";
    }
    return body.str();
}

BootStepResult DisabledWatchdog::arm() {
    return {"Watchdog", BootStepStatus::Warning, 0, "watchdog disabled by configuration"};
}

void DisabledWatchdog::kick() {}

void DisabledWatchdog::disarm() {}

BootFirmware::BootFirmware(
    BootConfig config,
    HardwareDiagnostics& diagnostics,
    SettingsStore& settings_store,
    StructuredLogger& logger,
    Watchdog& watchdog,
    Display* display)
    : config_(std::move(config)),
      diagnostics_(diagnostics),
      settings_store_(settings_store),
      logger_(logger),
      watchdog_(watchdog),
      display_(display) {}

BootReport BootFirmware::cold_boot() {
    BootReport report;
    boot_started_ = std::chrono::steady_clock::now();

    logger_.open();
    logger_.log(LogLevel::Info, "system", "boot_start", "cold boot started");

    render_boot_frame("Libra constellation", 5);
    record_step(&report, watchdog_.arm());
    watchdog_.kick();

    render_boot_frame("Loading D: Drive...", 12);
    BootStepResult settings_step{"Settings", BootStepStatus::Ok, 0, "settings loaded from SQLite"};
    const auto settings_started = std::chrono::steady_clock::now();
    if (!settings_store_.open() || !settings_store_.migrate()) {
        settings_step.status = BootStepStatus::Failed;
        settings_step.error_code = ErrorCode::SettingsMigration;
        settings_step.message = settings_store_.last_error();
        report.usable = false;
    } else {
        report.settings = settings_store_.load_runtime_settings();
        BootRecoveryManager recovery(settings_store_);
        const BootRecoveryStatus recovery_status = recovery.begin_boot();
        report.safe_mode = recovery_status.safe_mode;
        report.boot_counter = recovery_status.boot_counter;
        report.crash_counter = recovery_status.crash_counter;
        if (recovery_status.safe_mode) {
            report.settings.active_theme = "archive_dark";
            report.settings.power_mode = "normal";
        }
        record_step(&report, {
            "Crash Recovery",
            recovery_status.safe_mode ? BootStepStatus::Warning : BootStepStatus::Ok,
            0,
            recovery_status.safe_mode ? "safe mode enabled: " + recovery_status.reason : "last boot state checked",
            recovery_status.safe_mode ? ErrorCode::CrashLoop : ErrorCode::Ok,
        });
    }
    settings_step.elapsed_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - settings_started).count());
    record_step(&report, settings_step);

    const std::vector<std::pair<std::string, BootStepResult (*)(HardwareDiagnostics&)>> steps = {
        {"GPIO", call_gpio},
        {"SPI", call_spi},
        {"I2C", call_i2c},
        {"ALSA", call_alsa},
        {"Bluetooth", call_bluetooth},
        {"WiFi", call_wifi},
        {"Battery Gauge", call_battery},
        {"Display", call_display},
    };

    int progress = 20;
    for (const auto& step : steps) {
        render_boot_frame("mhm mhm / " + step.first, progress);
        record_step(&report, timed_step(step.first, step.second));
        watchdog_.kick();
        progress += 9;
    }

    render_boot_frame("SHAeR Home Screen", 100);
    report.total_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - boot_started_).count());

    const BootStepStatus timing_status = report.total_ms <= config_.boot_target_ms
        ? BootStepStatus::Ok
        : BootStepStatus::Warning;
    record_step(&report, {
        "Startup Timing",
        timing_status,
        report.total_ms,
        report.total_ms <= config_.boot_target_ms ? "within target" : "boot exceeded target",
    });

    logger_.log(LogLevel::Info, "system", "boot_complete", "cold boot completed", {
        {"total_ms", std::to_string(report.total_ms)},
        {"target_ms", std::to_string(config_.boot_target_ms)},
        {"usable", report.usable ? "true" : "false"},
    });
    return report;
}

void BootFirmware::clean_shutdown(const std::string& reason) {
    logger_.log(LogLevel::Info, "system", "shutdown_start", "clean shutdown requested", {
        {"reason", reason},
    });
    watchdog_.kick();
    mark_boot_successful();
    watchdog_.disarm();
    logger_.log(LogLevel::Info, "system", "shutdown_ready", "logs flushed and watchdog disarmed");
}

void BootFirmware::mark_boot_successful() {
    BootRecoveryManager recovery(settings_store_);
    recovery.mark_boot_successful();
    logger_.log(LogLevel::Info, "system", "boot_mark_successful", "boot marked successful");
}

BootStepResult BootFirmware::timed_step(
    const std::string& name,
    BootStepResult (*fn)(HardwareDiagnostics&)) {
    const auto started = std::chrono::steady_clock::now();
    BootStepResult result = fn(diagnostics_);
    result.name = name;
    result.elapsed_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
    return result;
}

void BootFirmware::record_step(BootReport* report, BootStepResult step) {
    step.error_code = code_for_step(step);
    if (step.status == BootStepStatus::Failed) {
        report->usable = false;
    }
    log_step(step);
    report->steps.push_back(std::move(step));
}

void BootFirmware::render_boot_frame(const std::string& message, int progress_percent) {
    if (!display_) {
        return;
    }
    RenderModel model;
    model.firmware_state = FirmwareState::Booting;
    model.screen = Screen::Boot;
    model.theme = {
        "archive_dark",
        "Archive Dark",
        "Libra constellation boot",
        "starfield resolve",
        "constellation trace",
        "anticipation and curiosity",
        ThemeDefinition{},
    };
    model.blueprint = {
        "Libra constellation",
        "Loading D: Drive...",
        "boot progress",
        "cold glow",
        {"Libra constellation", "Loading D: Drive...", "mhm mhm", message},
    };
    model.battery_percent = std::clamp(progress_percent, 0, 100);
    model.console = {"[Boot] " + message};
    display_->render(model);
}

void BootFirmware::log_step(const BootStepResult& step) {
    const LogLevel level = step.status == BootStepStatus::Ok
        ? LogLevel::Info
        : step.status == BootStepStatus::Warning ? LogLevel::Warning : LogLevel::Error;
    logger_.log(level, "system", "boot_step", step.message, {
        {"step", step.name},
        {"code", step.error_code},
        {"status", status_name(step.status)},
        {"elapsed_ms", std::to_string(step.elapsed_ms)},
    });
}

}  // namespace shaer
