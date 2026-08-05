#include "pi_system.hpp"

#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <utility>

namespace shaer {

BootStepResult PiDiagnostics::check_path(
    const std::string& name,
    const std::string& path,
    const std::string& ok_message,
    const std::string& fail_message) const {
    if (access(path.c_str(), F_OK) == 0) {
        return {name, BootStepStatus::Ok, 0, ok_message};
    }
    return {name, BootStepStatus::Failed, 0, fail_message + ": " + path};
}

BootStepResult PiDiagnostics::initialize_gpio() {
    return check_path("GPIO", "/sys/class/gpio/export", "GPIO sysfs available", "GPIO export missing");
}

BootStepResult PiDiagnostics::initialize_spi() {
    return check_path("SPI", "/dev/spidev0.0", "SPI display bus available", "SPI is not enabled");
}

BootStepResult PiDiagnostics::initialize_i2c() {
    return check_path("I2C", "/dev/i2c-1", "I2C bus available", "I2C is not enabled");
}

BootStepResult PiDiagnostics::initialize_alsa() {
    return check_path("ALSA", "/proc/asound/cards", "ALSA card registry available", "ALSA registry missing");
}

BootStepResult PiDiagnostics::initialize_bluetooth() {
    return check_path("Bluetooth", "/sys/class/bluetooth", "Bluetooth subsystem available", "Bluetooth subsystem missing");
}

BootStepResult PiDiagnostics::initialize_wifi() {
    return check_path("WiFi", "/sys/class/net/wlan0", "WiFi interface wlan0 available", "WiFi interface wlan0 missing");
}

BootStepResult PiDiagnostics::initialize_battery_gauge() {
    return check_path("Battery Gauge", "/dev/i2c-1", "MAX17048 can be probed over I2C", "Battery gauge bus unavailable");
}

BootStepResult PiDiagnostics::initialize_display() {
    return check_path("Display", "/dev/spidev0.0", "SPI display device available", "Display SPI device unavailable");
}

LinuxWatchdog::LinuxWatchdog(std::string path) : path_(std::move(path)) {}

LinuxWatchdog::~LinuxWatchdog() {
    disarm();
}

BootStepResult LinuxWatchdog::arm() {
    fd_ = open(path_.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd_ < 0) {
        return {"Watchdog", BootStepStatus::Warning, 0, "Linux watchdog unavailable at " + path_};
    }
    kick();
    return {"Watchdog", BootStepStatus::Ok, 0, "Linux watchdog armed"};
}

void LinuxWatchdog::kick() {
    if (fd_ < 0) {
        return;
    }
    const char value = '\0';
    write(fd_, &value, 1);
}

void LinuxWatchdog::disarm() {
    if (fd_ < 0) {
        return;
    }
    const char disable = 'V';
    write(fd_, &disable, 1);
    close(fd_);
    fd_ = -1;
}

}  // namespace shaer
