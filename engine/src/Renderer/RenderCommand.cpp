#include "ArcEngine/Renderer/RenderCommand.h"
#include <glad/gl.h>

namespace Arc {

Vec4 RenderCommand::s_clearColor(0.08f, 0.10f, 0.14f, 1.0f);

void RenderCommand::SetClearColor(const Vec4& color) {
    s_clearColor = color;
}

void RenderCommand::Clear() {
    glClearColor(s_clearColor.r, s_clearColor.g, s_clearColor.b, s_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glViewport(static_cast<int>(x), static_cast<int>(y),
               static_cast<int>(w), static_cast<int>(h));
}

} // namespace Arc
