#pragma once

#include <string>

namespace shaer {

// RendererAPI v1 is deliberately backend-neutral. UI code asks for primitive
// drawing/presentation operations; SDL, Pi framebuffer, and test renderers can
// implement them without changing app state or navigation logic.
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void begin_frame() = 0;
    virtual void draw_text(const std::string& text) = 0;
    virtual void present() = 0;
};

}  // namespace shaer

