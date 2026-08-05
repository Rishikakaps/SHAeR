#pragma once

#include "boot_firmware.hpp"

namespace shaer {

class DesktopDiagnostics final : public HardwareDiagnostics {
public:
    BootStepResult initialize_gpio() override;
    BootStepResult initialize_spi() override;
    BootStepResult initialize_i2c() override;
    BootStepResult initialize_alsa() override;
    BootStepResult initialize_bluetooth() override;
    BootStepResult initialize_wifi() override;
    BootStepResult initialize_battery_gauge() override;
    BootStepResult initialize_display() override;
};

class DesktopWatchdog final : public Watchdog {
public:
    BootStepResult arm() override;
    void kick() override;
    void disarm() override;
};

}  // namespace shaer

