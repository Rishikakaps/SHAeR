#include "theme_engine.hpp"

#include <cassert>
#include <iostream>

int main() {
    shaer::ThemeAssets assets("assets/themes/default");
    shaer::ThemeDefinition missing;
    missing.id = "missing";
    missing.resources.root = "assets/themes/does_not_exist";
    const auto resources = assets.load_active(missing);
    assert(resources.root == "assets/themes/default");
    const auto fallback = assets.resolve_asset(resources, "icons/song.png");
    assert(fallback.find("assets/themes/default") != std::string::npos);
    std::cout << "asset_loader_tests passed\n";
    return 0;
}
