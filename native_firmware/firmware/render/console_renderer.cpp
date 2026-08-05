#include "console_renderer.hpp"

#include <iostream>

namespace shaer {

ConsoleRenderer::ConsoleRenderer(std::ostream& out) : out_(out) {}

void ConsoleRenderer::begin_frame() {
    out_ << "\n\n";
}

void ConsoleRenderer::draw_text(const std::string& text) {
    out_ << text << "\n";
}

void ConsoleRenderer::present() {
    out_ << "> " << std::flush;
}

}  // namespace shaer

