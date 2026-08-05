#include "boot_firmware.hpp"

#include <cassert>
#include <filesystem>
#include <string>
#include <unistd.h>

namespace {

class TestDisplay final : public shaer::Display {
public:
    void render(const shaer::RenderModel& model) override {
        ++frames;
        last_screen = model.screen;
    }
    int frames = 0;
    shaer::Screen last_screen = shaer::Screen::Home;
};

class TestDiagnostics final : public shaer::HardwareDiagnostics {
public:
    shaer::BootStepResult initialize_gpio() override { return ok("GPIO"); }
    shaer::BootStepResult initialize_spi() override { return ok("SPI"); }
    shaer::BootStepResult initialize_i2c() override { return ok("I2C"); }
    shaer::BootStepResult initialize_alsa() override { return ok("ALSA"); }
    shaer::BootStepResult initialize_bluetooth() override { return ok("Bluetooth"); }
    shaer::BootStepResult initialize_wifi() override { return {"WiFi", shaer::BootStepStatus::Warning, 0, "wlan0 not connected"}; }
    shaer::BootStepResult initialize_battery_gauge() override { return ok("Battery Gauge"); }
    shaer::BootStepResult initialize_display() override { return ok("Display"); }

private:
    shaer::BootStepResult ok(const std::string& name) {
        return {name, shaer::BootStepStatus::Ok, 0, name + " ready"};
    }
};

class TestWatchdog final : public shaer::Watchdog {
public:
    shaer::BootStepResult arm() override {
        armed = true;
        return {"Watchdog", shaer::BootStepStatus::Ok, 0, "armed"};
    }
    void kick() override { ++kicks; }
    void disarm() override { armed = false; }
    bool armed = false;
    int kicks = 0;
};

}  // namespace

int main() {
    const std::string dir = "/tmp/shaer_boot_test_" + std::to_string(getpid());
    std::filesystem::remove_all(dir);

    TestDisplay display;
    TestDiagnostics diagnostics;
    TestWatchdog watchdog;
    shaer::SettingsStore settings(dir + "/data/library.db");
    shaer::StructuredLogger logger(dir + "/logs");

    shaer::BootFirmware boot(
        {dir + "/data", dir + "/logs", 8000, true},
        diagnostics,
        settings,
        logger,
        watchdog,
        &display);

    const auto report = boot.cold_boot();
    assert(report.usable);
    assert(report.has_failures());
    assert(report.diagnostic_body().find("WiFi") != std::string::npos);
    assert(report.total_ms >= 0);
    assert(display.frames >= 4);
    assert(display.last_screen == shaer::Screen::Boot);
    assert(watchdog.armed);
    assert(watchdog.kicks > 0);

    boot.clean_shutdown("unit test");
    assert(!watchdog.armed);

    const std::string crash_dir = dir + "/crash_loop";
    std::filesystem::create_directories(crash_dir);
    for (int i = 0; i < 4; ++i) {
        TestDisplay crash_display;
        TestDiagnostics crash_diagnostics;
        TestWatchdog crash_watchdog;
        shaer::SettingsStore crash_settings(crash_dir + "/settings.db");
        shaer::StructuredLogger crash_logger(crash_dir + "/logs_" + std::to_string(i));
        shaer::BootFirmware crash_boot(
            {crash_dir, crash_dir + "/logs_" + std::to_string(i), 8000, true},
            crash_diagnostics,
            crash_settings,
            crash_logger,
            crash_watchdog,
            &crash_display);
        const auto crash_report = crash_boot.cold_boot();
        if (i < 3) {
            assert(!crash_report.safe_mode);
        } else {
            assert(crash_report.safe_mode);
            assert(crash_report.crash_counter >= 3);
            assert(crash_report.settings.active_theme == "archive_dark");
            assert(crash_report.diagnostic_body().find("Crash Recovery") != std::string::npos ||
                   crash_report.has_failures());
        }
    }

    std::filesystem::remove_all(dir);
    return 0;
}
