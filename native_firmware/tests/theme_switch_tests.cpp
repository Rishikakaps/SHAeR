#include "theme_engine.hpp"

#include <cassert>
#include <iostream>

int main() {
    shaer::ThemeEngine engine;
    engine.set_theme("archive_dark");
    const auto archive = engine.render_profile();
    assert(archive.id == "archive_dark");
    engine.set_theme("not_installed");
    assert(engine.render_profile().id == "default");
    engine.cycle_theme();
    assert(!engine.render_profile().id.empty());
    std::cout << "theme_switch_tests passed\n";
    return 0;
}
