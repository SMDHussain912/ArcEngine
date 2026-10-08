// ArcRuntime (P0): runs the production App loop.
// Same feel as the M1 demo (pulse + FPS title + ESC) but the loop is App's
// fixed-step engine loop now: physics steps are COUNTED and shown in title.
// Usage: ArcRuntime [--frames N]   (N caps frames — used by CI verification)
#include "ArcEngine/Core/App.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Version.h"

#include <glad/gl.h>
#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    Arc::Log::Init();
    Arc::Log::Info(std::string("ArcEngine v") + Arc::GetVersion());

    Arc::AppConfig cfg;
    cfg.Title = "ArcEngine — P0 App loop";
    cfg.Width = 1280;
    cfg.Height = 720;
    cfg.PhysicsHz = 60.0f;
    cfg.QuitOnEscape = true;

    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--frames" && i + 1 < argc)
            cfg.MaxFrames = static_cast<uint64_t>(std::strtoull(argv[i + 1], nullptr, 10));
    }

    Arc::App app(cfg);
    double titleTimer = 0.0;

    // P0 Step 1: physics hook is a counted no-op — Scene lands with the .arc step.
    app.OnPhysics([](float fixedDt) { (void)fixedDt; });

    app.OnUpdate([&](float dt) {
        titleTimer += dt;
        if (titleTimer >= 0.5) {
            titleTimer = 0.0;
            int fps = static_cast<int>(app.Fps() + 0.5f);
            app.GetWindow()->SetTitle(
                "ArcEngine — P0 | " + std::to_string(fps) + " FPS | " +
                std::to_string(app.PhysicsSteps()) + " phys steps");
        }
    });

    app.OnRender([&](float alpha) {
        (void)alpha;
        // Slow clear-color pulse proves the render hook runs every frame.
        float t = static_cast<float>(app.Elapsed());
        float r = 0.15f + 0.10f * t - 0.10f * static_cast<int>(t);
        if (r > 0.35f) r = 0.15f;
        glClearColor(r, 0.20f, 0.30f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    });

    app.OnShutdown([] { Arc::Log::Info("App shutdown hook."); });

    return app.Run();
}

