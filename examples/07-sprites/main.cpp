// P1 example: SpriteBatch draws a layered sprite scene in ONE indexed draw
// per texture. WASD moves the player sprite, arrows move the enemy, Q/E flip,
// Z/X switch layers, ESC quits. Reports bind count per frame.
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Renderer/OpenGLRenderer.h"
#include "ArcEngine/Renderer/RenderCommand.h"
#include "ArcEngine/Renderer/SpriteBatch.h"
#include "ArcEngine/Scene/Scene.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>

int main() {
    Arc::Log::Init();
    Arc::Log::Info("07-sprites: batched sprites. WASD player, arrows enemy, Q/E flip, "
                   "Z/X layer, ESC quits.");

    Arc::Window window;
    if (!window.IsValid()) return 1;
    if (!Arc::GraphicsContext::Init()) return 1;
    Arc::Input::Init(window.NativeHandle());

    Arc::Renderer* renderer = Arc::CreateRenderer();
    renderer->SetViewport(0, 0, window.GetWidth(), window.GetHeight());
    Arc::RenderCommand::SetClearColor(Arc::Vec4(0.08f, 0.10f, 0.14f, 1.0f));

    Arc::Scene scene;
    Arc::Entity bg = scene.CreateEntity("Background");
    auto& bgS = bg.AddComponent<Arc::SpriteComponent>();
    bgS.TexturePath = "assets/textures/test.png";
    bgS.Size = {4.0f, 2.5f};
    bgS.Tint = Arc::Vec4(0.5f, 0.6f, 0.8f, 1.0f);
    bgS.Layer = -10;

    Arc::Entity player = scene.CreateEntity("Player");
    auto& ps = player.AddComponent<Arc::SpriteComponent>();
    ps.TexturePath = "assets/textures/test.png";
    ps.Size = {1.0f, 1.0f};
    ps.Layer = 1;
    player.GetComponent<Arc::TransformComponent>().Position = {-0.8f, 0.0f, 0.0f};

    Arc::Entity enemy = scene.CreateEntity("Enemy");
    auto& es = enemy.AddComponent<Arc::SpriteComponent>();
    es.TexturePath = "assets/textures/test.png";
    es.Tint = Arc::Vec4(1.0f, 0.4f, 0.4f, 1.0f);
    es.Size = {0.7f, 0.7f};
    es.Layer = 2;
    enemy.GetComponent<Arc::TransformComponent>().Position = {0.8f, 0.0f, 0.0f};

    // Atlas sub-region sprite: top-left quadrant of the texture.
    Arc::Entity atlas = scene.CreateEntity("AtlasBit");
    auto& as = atlas.AddComponent<Arc::SpriteComponent>();
    as.TexturePath = "assets/textures/test.png";
    as.RegionMin = {0.0f, 0.5f};
    as.RegionMax = {0.5f, 1.0f};
    as.Size = {0.5f, 0.5f};
    as.Layer = 3;
    atlas.GetComponent<Arc::TransformComponent>().Position = {0.0f, 0.9f, 0.0f};

    Arc::SpriteBatch batch;
    Arc::Mat4 vp = Arc::Ortho(-2.2f, 2.2f, -1.4f, 1.4f, -10.0f, 10.0f);

    Arc::Time clock;
    uint64_t frames = 0;
    while (!window.ShouldClose()) {
        clock.Tick();
        float dt = clock.Delta();

        auto& pt = player.GetComponent<Arc::TransformComponent>();
        if (Arc::Input::IsKeyDown(Arc::Key::A)) pt.Position.x -= 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::D)) pt.Position.x += 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::W)) pt.Position.y += 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::S)) pt.Position.y -= 1.5f * dt;
        auto& et = enemy.GetComponent<Arc::TransformComponent>();
        if (Arc::Input::IsKeyDown(Arc::Key::Left)) et.Position.x -= 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Right)) et.Position.x += 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Up)) et.Position.y += 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Down)) et.Position.y -= 1.5f * dt;
        if (Arc::Input::IsKeyPressed(Arc::Key::Q)) {
            ps.FlipX = !ps.FlipX;
            Arc::Log::Info(std::string("flipX=") + (ps.FlipX ? "on" : "off"));
        }
        if (Arc::Input::IsKeyPressed(Arc::Key::E)) {
            ps.FlipY = !ps.FlipY;
            Arc::Log::Info(std::string("flipY=") + (ps.FlipY ? "on" : "off"));
        }
        if (Arc::Input::IsKeyPressed(Arc::Key::Z)) {
            ps.Layer--;
            Arc::Log::Info("player layer=" + std::to_string(ps.Layer));
        }
        if (Arc::Input::IsKeyPressed(Arc::Key::X)) {
            ps.Layer++;
            Arc::Log::Info("player layer=" + std::to_string(ps.Layer));
        }
        if (Arc::Input::IsKeyDown(Arc::Key::Escape)) break;

        renderer->BeginFrame(Arc::RenderCommand::ClearColor());
        batch.Draw(scene, vp);
        renderer->EndFrame();

        if ((++frames % 120) == 0)
            Arc::Log::Info("sprites=" + std::to_string(batch.LastSpriteCount()) +
                           " binds=" + std::to_string(batch.LastBindCount()));

        window.SwapBuffers();
        window.PollEvents();
        Arc::Input::EndFrame();
    }

    Arc::Input::Shutdown();
    Arc::Log::Info("07-sprites: closed cleanly.");
    return 0;
}
