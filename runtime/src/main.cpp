#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Version.h"
#include "ArcEngine/Core/Window.h"

#include <GLFW/glfw3.h>
#include <string>

int main() {
    Arc::Log::Init();
    Arc::Log::Info(std::string("ArcEngine v") + Arc::GetVersion());

    Arc::WindowProps props;
    props.Title = "ArcEngine — M1 Window";
    props.Width = 1280;
    props.Height = 720;

    Arc::Window window(props);
    if (!window.IsValid()) {
        Arc::Log::Error("Failed to create window. Exiting.");
        return 1;
    }

    Arc::Time clock;
    double titleTimer = 0.0;

    // M1 loop: clear color pulses slowly so we can SEE the loop is alive
    // without needing shaders/buffers yet (those come in M2).
    while (!window.ShouldClose()) {
        clock.Tick();

        // Escape closes (Unity-like quick exit for dev)
        GLFWwindow* native = window.NativeHandle();
        if (glfwGetKey(native, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(native, GLFW_TRUE);

        float t = static_cast<float>(clock.Elapsed());
        float r = 0.15f + 0.10f * t - 0.10f * static_cast<int>(t); // slow pulse, no <cmath> needed
        if (r > 0.35f) r = 0.15f;
        glClearColor(r, 0.20f, 0.30f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.SwapBuffers();
        window.PollEvents();

        titleTimer += clock.Delta();
        if (titleTimer >= 0.5) {
            titleTimer = 0.0;
            int fps = static_cast<int>(clock.Fps() + 0.5f);
            window.SetTitle("ArcEngine — M1 Window | " + std::to_string(fps) + " FPS");
        }
    }

    Arc::Log::Info("Window closed. Goodbye!");
    return 0;
}

