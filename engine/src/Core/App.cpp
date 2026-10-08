#include "ArcEngine/Core/App.h"
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Project.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Scene/SceneSerializer.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cmath>
#include <filesystem>

namespace Arc {

App::App(AppConfig cfg) : m_cfg(std::move(cfg)), m_editorAvailable(true) {}

App::~App() {
    if (m_editorAvailable) m_editor.Shutdown();
}

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

    // Asset search paths: project dir wins, cwd is the fallback.
    if (m_project && !m_project->FilePath.empty())
        m_assets.AddSearchPath(
            std::filesystem::path(m_project->FilePath).parent_path().string());
    m_assets.AddSearchPath(".");

    // Main scene (resolved through AssetManager search paths).
    if (m_project && !m_project->MainScene.empty()) {
        std::string scenePath = m_assets.Resolve(m_project->MainScene);
        if (scenePath.empty()) scenePath = m_project->MainScene; // clear error below
        if (SceneSerializer::Load(scenePath, m_scene))
            Log::Info("App: main scene entities = " + std::to_string(m_scene.EntityCount()));
        else
            Log::Warn("App: main scene failed to load: " + m_project->MainScene);
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
    if (m_project)
        Log::Info("App: project '" + m_project->Name + "' v" + m_project->Version +
                  (m_project->MainScene.empty() ? "" : ", main scene: " + m_project->MainScene));

    // M7: editor window. Renders on top of the game view; F1 toggles.
    if (m_editorAvailable) {
        if (!m_editor.Init(*m_window, renderer(), m_scene))
            Log::Warn("App: ArcEditor init failed; continuing without editor.");
        else
            Log::Info("App: ArcEditor initialized (ImGui + GLFW + OpenGL3).");
    }

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
        // Paused with the editor world: no stepping while holding or paused.
        bool worldRunning =
            !m_editor.IsInitialized() || !m_editorVisible || m_editor.IsPlaying();
        if (worldRunning) m_accumulator += static_cast<double>(m_clock.Delta());
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
        // Editor play mode pauses the game world: updates run only when playing
        // (or when the editor overlay is hidden/off).
        bool editorHolding = m_editor.IsInitialized() && m_editorVisible && !m_editor.IsPlaying();
        if (m_onUpdate && !editorHolding) m_onUpdate(m_clock.Delta());
        const float alpha = static_cast<float>(m_accumulator / step);
        if (m_onRender) m_onRender(alpha);

        // M7: editor overlay (panels on top of the game view; F1 toggles).
        if (m_editorAvailable && m_editor.IsInitialized()) {
            if (Input::IsKeyPressed(Key::F1)) m_editorVisible = !m_editorVisible;
            if (m_editorVisible) {
                m_editor.BeginFrame(m_clock.Delta());
                m_editor.DrawAll();
                m_editor.EndFrame();
            }
        }

        // 5. Present + input edge bookkeeping for next frame.
        m_window->SwapBuffers();
        Input::EndFrame();
        m_frames++;
        if (m_cfg.MaxFrames > 0 && m_frames % 100 == 0 && m_frames < m_cfg.MaxFrames)
            Log::Info("App: " + std::to_string(m_frames) + "/" +
                      std::to_string(m_cfg.MaxFrames) + " frames...");
    }

    // Shutdown: user first (later: Script->Physics->Audio), then editor GL
    // resources while the context is still current, then Input, Window RAII.
    if (m_onShutdown) m_onShutdown();
    if (m_editorAvailable) m_editor.Shutdown();
    Input::Shutdown();
    Log::Info("App: stopped after " + std::to_string(m_frames) + " frames, " +
              std::to_string(m_physicsSteps) + " physics steps.");
    m_window.reset();
    m_scene.Clear();
    return 0;
}

} // namespace Arc
