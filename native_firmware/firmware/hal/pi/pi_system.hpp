#pragma once

#include "boot_firmware.hpp"

namespace shaer {

class PiDiagnostics final : public HardwareDiagnostics {
public:
    BootStepResult initialize_gpio() override;
    BootStepResult initialize_spi() override;
    BootStepResult initialize_i2c() override;
    BootStepResult initialize_alsa() override;
    BootStepResult initialize_bluetooth() override;
    BootStepResult initialize_wifi() override;
    BootStepResult initialize_battery_gauge() override;
    BootStepResult initialize_display() override;

private:
    BootStepResult check_path(const std::string& name, const std::string& path, const std::string& ok_message, const std::string& fail_message) const;
};

class LinuxWatchdog final : public Watchdog {
public:
    explicit LinuxWatchdog(std::string path = "/dev/watchdog");
    ~LinuxWatchdog() override;

    BootStepResult arm() override;
    void kick() override;
    void disarm() override;

private:
    std::string path_;
    int fd_ = -1;
};

}  // namespace shaer

