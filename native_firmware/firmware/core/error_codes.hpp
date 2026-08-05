#pragma once

namespace shaer {

namespace ErrorCode {

constexpr const char* Ok = "OK";
constexpr const char* BatteryMissing = "E001";
constexpr const char* SpotifyLogin = "E002";
constexpr const char* DacInit = "E003";
constexpr const char* SdCard = "E004";
constexpr const char* Wifi = "E005";
constexpr const char* Bluetooth = "E006";
constexpr const char* Display = "E007";
constexpr const char* Gpio = "E008";
constexpr const char* Spi = "E009";
constexpr const char* I2c = "E010";
constexpr const char* Watchdog = "E011";
constexpr const char* SettingsMigration = "E012";
constexpr const char* CrashLoop = "E013";
constexpr const char* Alsa = "E014";

}

}  // namespace shaer
