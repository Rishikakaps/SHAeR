#include "theme_engine.hpp"
#include "ui_framework.hpp"

#include <cassert>
#include <chrono>
#include <iostream>

int main() {
    shaer::ThemeEngine engine;
    shaer::UiFramework ui;
    shaer::RenderModel model;
    model.screen = shaer::Screen::Home;
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < 500; ++i) {
        engine.set_theme(i % 2 == 0 ? "default" : "archive_dark");
        model.theme = engine.render_profile();
        const auto frame = ui.build_frame(model);
        assert(!frame.commands.empty());
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    assert(elapsed < 1500);
    std::cout << "theme_performance_tests passed elapsed_ms=" << elapsed << "\n";
    return 0;
}
