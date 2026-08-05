#include "ui_framework.hpp"

#include <cassert>
#include <iostream>

int main() {
    assert(shaer::FontMetrics::glyph_width(1) == 6);
    assert(shaer::FontMetrics::glyph_height(2) == 14);
    assert(shaer::FontMetrics::text_width("SHAER", 1) == 30);
    assert(shaer::FontMetrics::fit_text("LOCAL LIBRARY", 78, 1) == "LOCAL LIBRARY");
    assert(shaer::FontMetrics::fit_text("A VERY LONG TRACK NAME", 60, 1) == "A VERY ...");
    std::cout << "font_tests passed\n";
    return 0;
}
