#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Core/Log.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Arc {

bool GraphicsContext::Init() {
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        Log::Error("gladLoadGL failed — no GL entry points. Wrong context version?");
        return false;
    }
    Log::Info(std::string("OpenGL loaded: ") + (const char*)glGetString(GL_VERSION));
    Log::Info(std::string("Renderer: ") + (const char*)glGetString(GL_RENDERER));
    return true;
}

const char* GraphicsContext::Version() {
    return (const char*)glGetString(GL_VERSION);
}

const char* GraphicsContext::Renderer() {
    return (const char*)glGetString(GL_RENDERER);
}

} // namespace Arc
