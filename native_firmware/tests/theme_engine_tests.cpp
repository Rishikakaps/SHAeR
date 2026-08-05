#include "theme_engine.hpp"

#include <cassert>
#include <iostream>

int main() {
    shaer::ThemeEngine engine;
    const auto& definition = engine.active_definition();
    assert(definition.id == "default");
    assert(!definition.typography.primary.empty());
    assert(!definition.typography.secondary.empty());
    assert(definition.animations.target_fps > 0);
    assert(definition.manifest.theme_version > 0);
    assert(definition.manifest.mandatory_screens.size() >= 17);
    assert(engine.validation().valid);
    const auto profile = engine.render_profile();
    assert(profile.id == definition.id);
    assert(profile.definition.palette.foreground.r > 0);
    const auto blueprint = engine.screen_blueprint(shaer::Screen::Home);
    assert(!blueprint.lines.empty());
    std::cout << "theme_engine_tests passed\n";
    return 0;
}
