#pragma once
#include "ArcEngine/Renderer/Renderer.h"

namespace Arc {

// Desktop backend (M4): OpenGL 4.6 Core via vendored glad.
// Owns nothing — Window owns the context, Shader/VAO bound by caller.
class OpenGLRenderer : public Renderer {
public:
    OpenGLRenderer();
    ~OpenGLRenderer() override = default;

    void BeginFrame(const Vec4& clearColor) override;
    void EndFrame() override;

    void DrawArrays(uint32_t vertexCount) override;
    void DrawIndexed(uint32_t indexCount) override;

    void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) override;
};

// Factory: returns the right backend for this platform.
// Linux -> OpenGLRenderer. Android (M8) -> OpenGLESRenderer.
Renderer* CreateRenderer();

} // namespace Arc
