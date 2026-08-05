#include "theme_engine.hpp"

#include <cassert>
#include <iostream>

int main() {
    shaer::ThemeEngine engine;
    engine.set_theme("default");
    const auto default_assets = engine.active_asset_count();
    engine.set_theme("archive_dark");
    const auto archive_assets = engine.active_asset_count();
    assert(engine.render_profile().id == "archive_dark");
    assert(archive_assets < 128);
    engine.set_theme("default");
    assert(engine.active_asset_count() == default_assets);
    std::cout << "theme_memory_tests passed default_assets=" << default_assets
              << " archive_assets=" << archive_assets << "\n";
    return 0;
}
