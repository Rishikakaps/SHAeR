#include "desktop_system.hpp"

namespace shaer {

namespace {

BootStepResult ok(const std::string& name, const std::string& message) {
    return {name, BootStepStatus::Ok, 0, message};
}

}  // namespace

BootStepResult DesktopDiagnostics::initialize_gpio() {
    return ok("GPIO", "desktop input backend ready");
}

BootStepResult DesktopDiagnostics::initialize_spi() {
    return ok("SPI", "desktop display backend ready");
}

BootStepResult DesktopDiagnostics::initialize_i2c() {
    return ok("I2C", "desktop battery backend ready");
}

BootStepResult DesktopDiagnostics::initialize_alsa() {
    return ok("ALSA", "desktop audio backend ready");
}

BootStepResult DesktopDiagnostics::initialize_bluetooth() {
    return ok("Bluetooth", "desktop bluetooth backend ready");
}

BootStepResult DesktopDiagnostics::initialize_wifi() {
    return ok("WiFi", "desktop network assumed ready");
}

BootStepResult DesktopDiagnostics::initialize_battery_gauge() {
    return ok("Battery Gauge", "desktop battery percentage available");
}

BootStepResult DesktopDiagnostics::initialize_display() {
    return ok("Display", "desktop renderer available");
}

BootStepResult DesktopWatchdog::arm() {
    return {"Watchdog", BootStepStatus::Ok, 0, "desktop watchdog armed"};
}

void DesktopWatchdog::kick() {}

void DesktopWatchdog::disarm() {}

}  // namespace shaer

