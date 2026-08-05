#pragma once

#include "renderer.hpp"

#include <iosfwd>

namespace shaer {

class ConsoleRenderer final : public Renderer {
public:
    explicit ConsoleRenderer(std::ostream& out);

    void begin_frame() override;
    void draw_text(const std::string& text) override;
    void present() override;

private:
    std::ostream& out_;
};

}  // namespace shaer

