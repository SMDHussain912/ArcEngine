#include "ArcEngine/Core/App.h"
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Renderer/GraphicsContext.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cmath>

namespace Arc {

App::App(AppConfig cfg) : m_cfg(std::move(cfg)) {}

App::~App() = default;

void App::RequestQuit() {
    m_quit = true;
    if (m_window) glfwSetWindowShouldClose(m_window->NativeHandle(), GLFW_TRUE);
}

int App::Run() {
    WindowProps props;
    props.Title = m_cfg.Title;
    props.Width = m_cfg.Width;
    props.Height = m_cfg.Height;
    props.VSync = m_cfg.VSync;

    m_window = std::make_unique<Window>(props);
    if (!m_window->IsValid()) {
        Log::Error("App: window creation failed.");
        return 1;
    }
    if (!GraphicsContext::Init()) return 1;
    Input::Init(m_window->NativeHandle());

    const double step = 1.0 / static_cast<double>(m_cfg.PhysicsHz);
    m_clock = Time();   // prime clock so pre-Run setup time isn't counted
    m_accumulator = 0.0;

    Log::Info("App loop: " + std::to_string(m_cfg.PhysicsHz) + " Hz physics, " +
              std::to_string(m_cfg.Width) + "x" + std::to_string(m_cfg.Height));
    if (m_cfg.MaxFrames > 0)
        Log::Info("App: frame cap = " + std::to_string(m_cfg.MaxFrames));

    while (!m_quit && !m_window->ShouldClose()) {
        if (m_cfg.MaxFrames > 0 && m_frames >= m_cfg.MaxFrames) {
            RequestQuit();
            break;
        }

        // 1. Clock + fresh input events (Godot Main::iteration order).
        m_clock.Tick();
        m_window->PollEvents();

        if (m_cfg.QuitOnEscape && Input::IsKeyDown(Key::Escape)) {
            RequestQuit();
            break;
        }

        // 2. Fixed-step physics with accumulator + spiral guard.
        m_accumulator += static_cast<double>(m_clock.Delta());
        int steps = 0;
        while (m_accumulator >= step && steps < m_cfg.MaxPhysicsSteps) {
            if (m_onPhysics) m_onPhysics(static_cast<float>(step));
            m_accumulator -= step;
            m_physicsSteps++;
            steps++;
        }
        if (m_accumulator >= step) {
            // Skip warmup frames: first SwapBuffers can stall driver init
            // (100ms+ on Mesa) — that's expected, not a real spiral.
            if (!m_warnedSpiral && m_frames > 5) {
                Log::Warn("App: physics spiral guard hit, dropping time ("
                          + std::to_string(steps) + " max steps).");
                m_warnedSpiral = true;
            }
            m_accumulator = std::fmod(m_accumulator, step);
        }

        // 3. Variable-rate update, 4. render with interpolation alpha.
        if (m_onUpdate) m_onUpdate(m_clock.Delta());
        const float alpha = static_cast<float>(m_accumulator / step);
        if (m_onRender) m_onRender(alpha);

        // 5. Present + input edge bookkeeping for next frame.
        m_window->SwapBuffers();
        Input::EndFrame();
        m_frames++;
        if (m_cfg.MaxFrames > 0 && m_frames % 100 == 0 && m_frames < m_cfg.MaxFrames)
            Log::Info("App: " + std::to_string(m_frames) + "/" +
                      std::to_string(m_cfg.MaxFrames) + " frames...");
    }

    // Shutdown: user first (later: Script->Physics->Audio), then Input, Window RAII.
    if (m_onShutdown) m_onShutdown();
    Input::Shutdown();
    Log::Info("App: stopped after " + std::to_string(m_frames) + " frames, " +
              std::to_string(m_physicsSteps) + " physics steps.");
    m_window.reset();
    return 0;
}

} // namespace Arc
