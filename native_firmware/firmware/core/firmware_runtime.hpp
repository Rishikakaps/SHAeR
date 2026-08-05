#pragma once

#include "boot_firmware.hpp"
#include "firmware_services.hpp"
#include "main_scheduler.hpp"

namespace shaer {

struct RuntimeHardware {
    Display& display;
    AudioOutput& audio;
    Input& input;
    Battery& battery;
    Bluetooth& bluetooth;
};

class FirmwareRuntime {
public:
    FirmwareRuntime(RuntimeHardware hardware, StructuredLogger* logger);
    const AppState& state() const;
    EventBus& events();
    void apply_boot_report(const BootReport& report);
    void run();
    void run_for_ticks(int ticks);
    void shutdown();

private:
    RuntimeHardware hardware_;
    StructuredLogger* logger_;
    EventBus events_;
    AppStateStore state_;
    MainScheduler scheduler_;
    bool initialized_ = false;
};

}  // namespace shaer
