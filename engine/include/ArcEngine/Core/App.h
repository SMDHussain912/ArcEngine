#pragma once
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace Arc {

struct Project;

struct AppConfig {
    std::string Title = "ArcEngine";
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool VSync = true;
    // Fixed-step physics rate (Godot/Unity both default 60Hz).
    float PhysicsHz = 60.0f;
    // Spiral-of-death guard: max fixed steps per frame before dropping time.
    int MaxPhysicsSteps = 5;
    bool QuitOnEscape = true;
    // Test/CI aid: quit after N frames (0 = run until user closes).
    uint64_t MaxFrames = 0;
};

// Production game loop (P0 Step 1). One ordered frame — Unreal lesson:
// subsystems never spin their own loops.
//
//   PollEvents -> fixed-step physics (accumulator) -> update(dt)
//   -> render(alpha) -> swap -> Input::EndFrame
//
// Callbacks are registered before Run(). Window/GL/Input init happens inside
// Run(); shutdown order is OnShutdown -> Input -> Window (RAII).
// Scene/.arc loading hooks in later P0 steps — App stays callback-based.
class App {
public:
    using UpdateFn = std::function<void(float dt)>;
    using PhysicsFn = std::function<void(float fixedDt)>;
    using RenderFn = std::function<void(float alpha)>;
    using ShutdownFn = std::function<void()>;

    explicit App(AppConfig cfg = {});
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void OnUpdate(UpdateFn fn) { m_onUpdate = std::move(fn); }
    void OnPhysics(PhysicsFn fn) { m_onPhysics = std::move(fn); }
    void OnRender(RenderFn fn) { m_onRender = std::move(fn); }
    void OnShutdown(ShutdownFn fn) { m_onShutdown = std::move(fn); }

    // Optional project context (P0 Step 2). Register before Run().
    // Future steps use it for asset root + main scene; Run() logs it now.
    void SetProject(std::shared_ptr<Project> p) { m_project = std::move(p); }
    Project* GetProject() { return m_project.get(); }

    // Blocks until quit. Returns process exit code.
    int Run();
    void RequestQuit();

    // Valid inside callbacks (null before Run / after it returns).
    Window* GetWindow() { return m_window.get(); }

    const AppConfig& Config() const { return m_cfg; }
    float Fps() const { return m_clock.Fps(); }
    uint64_t FrameCount() const { return m_frames; }
    uint64_t PhysicsSteps() const { return m_physicsSteps; }
    double Elapsed() const { return m_clock.Elapsed(); }

private:
    AppConfig m_cfg;
    std::unique_ptr<Window> m_window;
    std::shared_ptr<Project> m_project;
    Time m_clock;
    double m_accumulator = 0.0;
    uint64_t m_frames = 0;
    uint64_t m_physicsSteps = 0;
    bool m_quit = false;
    bool m_warnedSpiral = false;

    UpdateFn m_onUpdate;
    PhysicsFn m_onPhysics;
    RenderFn m_onRender;
    ShutdownFn m_onShutdown;
};

} // namespace Arc
