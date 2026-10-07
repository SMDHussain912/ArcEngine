#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Core/Log.h"

// M2: glad FIRST, then GLFW with no system GL headers.
// glad provides all GL entry points for 4.6 Core; GLFW must not pull gl.h.
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace Arc {

namespace {
int s_glfwRefCount = 0;

void ReleaseGlfw() {
    s_glfwRefCount--;
    if (s_glfwRefCount <= 0) {
        s_glfwRefCount = 0;
        glfwTerminate();
    }
}
} // namespace

Window::Window(const WindowProps& props) {
    Init(props);
}

Window::~Window() {
    Shutdown();
}

void Window::Init(const WindowProps& props) {
    m_props = props;

    if (s_glfwRefCount == 0) {
        glfwSetErrorCallback([](int code, const char* desc) {
            Log::Error(std::string("GLFW [") + std::to_string(code) + "]: " + (desc ? desc : ""));
        });
        if (!glfwInit()) {
            Log::Error("glfwInit() failed — Window will be invalid.");
            return;
        }
    }
    s_glfwRefCount++;

    // M2: request explicit OpenGL 4.6 Core. M1 used compat defaults.
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, props.GLMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, props.GLMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE,
                   props.GLCoreProfile ? GLFW_OPENGL_CORE_PROFILE : GLFW_OPENGL_ANY_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, props.Resizable ? GLFW_TRUE : GLFW_FALSE);

    m_handle = glfwCreateWindow(static_cast<int>(m_props.Width),
                                static_cast<int>(m_props.Height),
                                m_props.Title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        Log::Error("glfwCreateWindow failed.");
        ReleaseGlfw();
        return;
    }

    glfwMakeContextCurrent(m_handle);
    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, FramebufferSizeCallback);

    SetVSync(m_props.VSync);

    Log::Info("Window created: " + m_props.Title + " (" +
              std::to_string(m_props.Width) + "x" + std::to_string(m_props.Height) + ")");
}

void Window::Shutdown() {
    if (m_handle) {
        glfwDestroyWindow(m_handle);
        m_handle = nullptr;
        ReleaseGlfw();
    }
}

bool Window::ShouldClose() const {
    return m_handle ? glfwWindowShouldClose(m_handle) : true;
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    if (m_handle) glfwSwapBuffers(m_handle);
}

void Window::SetVSync(bool enabled) {
    m_props.VSync = enabled;
    // swap interval is per-context; only call with a current context
    if (m_handle) glfwSwapInterval(enabled ? 1 : 0);
}

void Window::SetTitle(const std::string& title) {
    m_props.Title = title;
    if (m_handle) glfwSetWindowTitle(m_handle, title.c_str());
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int w, int h) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_props.Width = static_cast<uint32_t>(w);
        self->m_props.Height = static_cast<uint32_t>(h);
    }
}

void Window::ErrorCallback(int code, const char* desc) {
    Log::Error(std::string("GLFW [") + std::to_string(code) + "]: " + (desc ? desc : ""));
}

} // namespace Arc
