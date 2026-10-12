// P1 Step 2 example: Box2D 3.x physics drives the sprites. A dynamic crate
// falls under gravity, lands on a static floor, and settles. SPACE launches a
// fresh crate from the top; the floor and walls stay put (static bodies). The
// physics world steps at a fixed 1/60 and writes poses back into transforms,
// so SpriteBatch just draws whatever physics produced. ESC quits.
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Physics/PhysicsWorld.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Renderer/OpenGLRenderer.h"
#include "ArcEngine/Renderer/RenderCommand.h"
#include "ArcEngine/Renderer/SpriteBatch.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>
#include <vector>

// Give an entity a textured sprite sized to its collider so you can see it.
static void MakeVisual(Arc::Entity e, const Arc::Vec4& tint, float w, float h) {
    auto& s = e.AddComponent<Arc::SpriteComponent>();
    s.TexturePath = "assets/textures/test.png"; // magenta fallback if absent
    s.Tint = tint;
    s.Size = {w, h};
}

int main() {
    Arc::Log::Init();
    Arc::Log::Info("08-physics: Box2D crate drops onto a static floor. "
                   "SPACE drops a new crate, ESC quits.");

    Arc::Window window;
    if (!window.IsValid()) return 1;
    if (!Arc::GraphicsContext::Init()) return 1;
    Arc::Input::Init(window.NativeHandle());

    Arc::Renderer* renderer = Arc::CreateRenderer();
    renderer->SetViewport(0, 0, window.GetWidth(), window.GetHeight());
    Arc::RenderCommand::SetClearColor(Arc::Vec4(0.07f, 0.08f, 0.11f, 1.0f));

    Arc::Scene scene;

    // Static floor: immovable, catches everything.
    Arc::Entity floor = scene.CreateEntity("Floor");
    floor.GetComponent<Arc::TransformComponent>().Position = {0.0f, -2.5f, 0.0f};
    auto& frb = floor.AddComponent<Arc::RigidBody2DComponent>();
    frb.Type = Arc::RigidBodyType::Static;
    auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
    fcol.Size = {9.0f, 0.5f};
    MakeVisual(floor, Arc::Vec4(0.4f, 0.45f, 0.5f, 1.0f), 9.0f, 0.5f);

    // A dynamic crate that falls, lands, and rests on the floor.
    Arc::Entity crate = scene.CreateEntity("Crate");
    crate.GetComponent<Arc::TransformComponent>().Position = {-1.5f, 2.0f, 0.0f};
    auto& crb = crate.AddComponent<Arc::RigidBody2DComponent>();
    crb.Type = Arc::RigidBodyType::Dynamic;
    auto& ccol = crate.AddComponent<Arc::BoxCollider2DComponent>();
    ccol.Size = {0.8f, 0.8f};
    ccol.Restitution = 0.2f; // a little bounce
    MakeVisual(crate, Arc::Vec4(0.95f, 0.6f, 0.25f, 1.0f), 0.8f, 0.8f);

    Arc::PhysicsWorld physics;
    physics.Create(); // default gravity (0, -9.8)
    for (auto e : scene.Entities()) physics.AddBody(scene, e);

    Arc::SpriteBatch batch;
    Arc::Mat4 vp = Arc::Ortho(-4.5f, 4.5f, -3.0f, 3.0f, -10.0f, 10.0f);
    Arc::Time clock;
    uint64_t frames = 0;
    std::vector<Arc::Entity> crates{crate};

    while (!window.ShouldClose()) {
        clock.Tick();
        float dt = clock.Delta();

        // SPACE: drop a new crate from a pseudo-random x near the top.
        if (Arc::Input::IsKeyPressed(Arc::Key::Space) && crates.size() < 24) {
            float x = -3.5f + 0.3f * static_cast<float>(crates.size() % 24);
            Arc::Entity c = scene.CreateEntity("Crate" + std::to_string(crates.size()));
            c.GetComponent<Arc::TransformComponent>().Position = {x, 3.0f, 0.0f};
            auto& rb = c.AddComponent<Arc::RigidBody2DComponent>();
            rb.Type = Arc::RigidBodyType::Dynamic;
            auto& col = c.AddComponent<Arc::BoxCollider2DComponent>();
            col.Size = {0.6f, 0.6f};
            col.Restitution = 0.3f;
            MakeVisual(c, Arc::Vec4(0.6f, 0.8f, 0.95f, 1.0f), 0.6f, 0.6f);
            physics.AddBody(scene, c);
            crates.push_back(c);
        }
        if (Arc::Input::IsKeyDown(Arc::Key::Escape)) break;

        // Physics first: advances the sim and writes poses into transforms,
        // then rendering reflects the new state this same frame.
        physics.Step(scene, dt);

        renderer->BeginFrame(Arc::RenderCommand::ClearColor());
        batch.Draw(scene, vp);
        renderer->EndFrame();

        if ((++frames % 120) == 0) {
            auto& t = crate.GetComponent<Arc::TransformComponent>();
            Arc::Log::Info("crates=" + std::to_string(crates.size()) +
                           " first.y=" + std::to_string(t.Position.y));
        }

        window.SwapBuffers();
        window.PollEvents();
        Arc::Input::EndFrame();
    }

    Arc::Input::Shutdown();
    Arc::Log::Info("08-physics: closed cleanly.");
    return 0;
}
