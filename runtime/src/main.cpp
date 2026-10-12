// ArcRuntime (P0): runs the production App loop.
// Usage: ArcRuntime [path/to/project.arc] [--frames N]
//   project.arc fills window/physics defaults (P0 Step 2);
//   --frames N caps frames for CI verification.
#include "ArcEngine/Core/App.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Project.h"
#include "ArcEngine/Core/Version.h"
#include "ArcEngine/Physics/PhysicsWorld.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Entity.h"
#include "ArcEngine/Scene/Scene.h"

#include <glad/gl.h>
#include <cstdlib>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    Arc::Log::Init();
    Arc::Log::Info(std::string("ArcEngine v") + Arc::GetVersion());

    std::string projectPath;
    Arc::AppConfig cfg;   // defaults when no project given
    cfg.Title = "ArcEngine — P0 App loop";
    uint64_t frameCap = 0;

    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        if (a == "--frames" && i + 1 < argc) {
            frameCap = std::strtoull(argv[i + 1], nullptr, 10);
            i++;
        } else if (!a.empty() && a[0] != '-') {
            projectPath = a;
        } else {
            Arc::Log::Warn("Unknown arg: " + a);
        }
    }

    std::shared_ptr<Arc::Project> project;
    if (!projectPath.empty()) {
        auto loaded = Arc::Project::Load(projectPath);
        if (!loaded) return 1;
        project = std::make_shared<Arc::Project>(std::move(*loaded));
        cfg = project->ToAppConfig();
    }
    cfg.MaxFrames = frameCap;

    Arc::App app(cfg);
    if (project) app.SetProject(project);
    double titleTimer = 0.0;

    // P1 Step 2: real Box2D world. Register every rigid entity once, then let
    // App's fixed-step loop drive it. Bodies write poses into transforms.
    Arc::PhysicsWorld physics;
    Arc::Scene& scene = app.GetScene();
    physics.Create();
    for (Arc::Entity e : scene.Entities())
        if (e.HasComponent<Arc::RigidBody2DComponent>()) physics.AddBody(scene, e);
    Arc::Log::Info("Runtime: " + std::to_string(physics.BodyCount()) +
                   " physics bodies registered.");

    app.OnPhysics([&](float fixedDt) {
        // PhysicsWorld::Step already runs its own fixed 1/60 accumulator; App
        // hands us a fixed dt, so one sim tick per physics callback.
        physics.Step(scene, fixedDt);
    });

    app.OnUpdate([&](float dt) {
        titleTimer += dt;
        if (titleTimer >= 0.5) {
            titleTimer = 0.0;
            int fps = static_cast<int>(app.Fps() + 0.5f);
            app.GetWindow()->SetTitle(
                app.Config().Title + " | " + std::to_string(fps) + " FPS | " +
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

