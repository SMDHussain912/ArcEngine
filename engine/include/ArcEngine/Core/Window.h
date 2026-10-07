#pragma once
#include <cstdint>
#include <string>

struct GLFWwindow;

namespace Arc {

struct WindowProps {
    std::string Title = "ArcEngine";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool VSync = true;
    bool Resizable = true;
};

// Thin RAII wrapper over GLFW (M1).
// M4 will generalize this into a platform interface so Android
// can swap GLFW -> EGL/NativeActivity without touching game code.
class Window {
public:
    explicit Window(const WindowProps& props = WindowProps{});
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool IsValid() const { return m_handle != nullptr; }
    bool ShouldClose() const;
    void PollEvents();
    void SwapBuffers();

    void SetVSync(bool enabled);
    void SetTitle(const std::string& title);

    uint32_t GetWidth() const { return m_props.Width; }
    uint32_t GetHeight() const { return m_props.Height; }
    GLFWwindow* NativeHandle() const { return m_handle; }

private:
    void Init(const WindowProps& props);
    void Shutdown();

    static void FramebufferSizeCallback(GLFWwindow* window, int w, int h);
    static void ErrorCallback(int code, const char* desc);

private:
    WindowProps m_props{};
    GLFWwindow* m_handle = nullptr;
};

} // namespace Arc
