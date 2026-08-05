#pragma once

#include "app_state.hpp"
#include "theme_engine.hpp"

namespace shaer {

class ScreenManager {
public:
    // Stable registration view consumed by diagnostics and future extension tooling.
    static const std::vector<ScreenRegistration>& registrations();
    RenderModel build_render_model(const AppState& state);

private:
    ThemeEngine themes_;
};

}  // namespace shaer
