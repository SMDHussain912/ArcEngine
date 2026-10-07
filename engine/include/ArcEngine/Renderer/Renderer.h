#pragma once
#include "ArcEngine/Core/Math.h"

namespace Arc {

enum class RendererAPI { None = 0, OpenGL = 1, OpenGLES = 2 };

// High-level draw commands game code uses (M4).
// No raw gl* outside Renderer backends from here on —
// Android (M8) only swaps the backend, not game code.
class Renderer {
public:
    virtual ~Renderer() = default;

    virtual void BeginFrame(const Vec4& clearColor) = 0;
    virtual void EndFrame() = 0;

    virtual void DrawArrays(uint32_t vertexCount) = 0;
    virtual void DrawIndexed(uint32_t indexCount) = 0;

    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) = 0;

    static RendererAPI GetAPI();
};

} // namespace Arc
