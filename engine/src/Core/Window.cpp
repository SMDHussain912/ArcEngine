#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Core/Log.h"

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

    // M1: plain OpenGL context window. M2 adds glad loader + version hints.
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_RESIZABLE, props.Resizable ? GLFW_TRUE : GLFW_FALSE);
    // Keep compat defaults for M1 so glClear works without a loader.
    // M2 will request 4.6 Core explicitly.

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
