// M1 example: minimal window using Arc::Window + Arc::Time + Arc::Log.
// Run: ./examples/01-window  (or ./runtime/ArcRuntime for the full runtime)
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/GraphicsContext.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

int main() {
    Arc::Log::Init();
    Arc::Log::Info("01-window: opening 960x540 window. Press ESC to close.");

    Arc::WindowProps props;
    props.Title = "ArcEngine — 01-window";
    props.Width = 960;
    props.Height = 540;

    Arc::Window window(props);
    if (!window.IsValid()) return 1;

    // M2: 4.6 Core needs glad entry points before any gl* call.
    if (!Arc::GraphicsContext::Init()) return 1;

    Arc::Time clock;
    while (!window.ShouldClose()) {
        clock.Tick();

        GLFWwindow* native = window.NativeHandle();
        if (glfwGetKey(native, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(native, GLFW_TRUE);

        glClearColor(0.12f, 0.16f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.SwapBuffers();
        window.PollEvents();
    }

    Arc::Log::Info("01-window: closed cleanly.");
    return 0;
}
