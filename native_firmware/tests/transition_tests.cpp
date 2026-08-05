#include "ui_framework.hpp"

#include <cassert>
#include <iostream>

int main() {
    shaer::RenderModel model;
    model.screen = shaer::Screen::Library;
    model.firmware_state = shaer::FirmwareState::LocalLibrary;
    model.transition = {shaer::Screen::Home, shaer::Screen::Library, "slide", 240, false, false, "navigation"};
    model.clock.valid = true;
    model.clock.time_12h = "07:45 PM";
    model.clock.date_label = "2026-06-30";
    model.power.display_budget_fps = 60;

    shaer::UiFramework ui;
    const auto frame = ui.build_frame(model);
    bool transition_found = false;
    for (const auto& command : frame.commands) {
        if (command.type == shaer::UiCommandType::Transition) {
            transition_found = true;
            assert(command.text == "slide");
            assert(command.value == 240);
            assert(command.rect.h == 2);
        }
    }
    assert(transition_found);

    model.transition.duration_ms = 0;
    const auto no_transition = ui.build_frame(model);
    for (const auto& command : no_transition.commands) {
        assert(command.type != shaer::UiCommandType::Transition);
    }

    std::cout << "transition_tests passed\n";
    return 0;
}
