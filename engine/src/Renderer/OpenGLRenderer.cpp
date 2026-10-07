#include "ArcEngine/Renderer/OpenGLRenderer.h"
#include <glad/gl.h>

namespace Arc {

RendererAPI Renderer::GetAPI() {
#ifdef ARC_GLES
    return RendererAPI::OpenGLES;
#else
    return RendererAPI::OpenGL;
#endif
}

OpenGLRenderer::OpenGLRenderer() {
    glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::BeginFrame(const Vec4& clearColor) {
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::EndFrame() {
    // SwapBuffers is Window's job (it owns the surface).
}

void OpenGLRenderer::DrawArrays(uint32_t vertexCount) {
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(vertexCount));
}

void OpenGLRenderer::DrawIndexed(uint32_t indexCount) {
    glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount), GL_UNSIGNED_INT, nullptr);
}

void OpenGLRenderer::SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    glViewport(static_cast<int>(x), static_cast<int>(y),
               static_cast<int>(w), static_cast<int>(h));
}

Renderer* CreateRenderer() {
    // Only backend for now. M8 adds: #ifdef __ANDROID__ return new OpenGLESRenderer();
    static OpenGLRenderer s_instance;
    return &s_instance;
}

} // namespace Arc
