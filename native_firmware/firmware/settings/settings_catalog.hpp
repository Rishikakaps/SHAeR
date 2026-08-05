#pragma once

#include "types.hpp"

#include <map>
#include <string>
#include <vector>

namespace shaer {

std::map<std::string, std::string> default_settings_values();
std::vector<SettingsCategory> build_settings_catalog(
    const RuntimeSettings& settings,
    const DeviceInfoSnapshot& device,
    int battery_percent,
    bool charging,
    const LibraryIndex& library);
std::string next_setting_value(const std::string& key, const std::string& current);
bool setting_is_action(const std::string& key);

}  // namespace shaer
