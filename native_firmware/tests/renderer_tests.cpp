#include "ui_framework.hpp"

#include <cassert>
#include <iostream>

int main() {
    shaer::RenderModel model;
    model.screen = shaer::Screen::Home;
    model.theme.definition.palette.background = {1, 2, 3};
    model.theme.definition.palette.foreground = {4, 5, 6};
    model.theme.definition.palette.selection = {7, 8, 9};
    shaer::UiFramework ui;
    const auto frame = ui.build_frame(model);
    assert(!frame.commands.empty());
    assert(frame.commands.front().fg.r == 1);
    assert(frame.commands.front().fg.g == 2);
    assert(frame.commands.front().fg.b == 3);
    bool saw_selection = false;
    for (const auto& command : frame.commands) {
        if (command.type == shaer::UiCommandType::Rect &&
            command.fg.r == 7 &&
            command.fg.g == 8 &&
            command.fg.b == 9) {
            saw_selection = true;
        }
    }
    assert(saw_selection);
    std::cout << "renderer_tests passed\n";
    return 0;
}
