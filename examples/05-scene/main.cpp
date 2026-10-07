// M5 example: Unity-like Scene + Entity + Transform + Mesh.
// Two entities orbit independently — same buffers, different u_Model per draw.
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Renderer/OpenGLRenderer.h"
#include "ArcEngine/Renderer/RenderCommand.h"
#include "ArcEngine/Renderer/Shader.h"
#include "ArcEngine/Scene/MeshComponent.h"
#include "ArcEngine/Scene/Scene.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
const char* kVert = R"(
#version 460 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Color;
out vec3 v_Color;
uniform mat4 u_Model;
void main() {
    v_Color = a_Color;
    gl_Position = u_Model * vec4(a_Pos, 1.0);
}
)";
const char* kFrag = R"(
#version 460 core
in vec3 v_Color;
out vec4 FragColor;
void main() { FragColor = vec4(v_Color, 1.0); }
)";
} // namespace

int main() {
    Arc::Log::Init();
    Arc::Log::Info("05-scene: two entities, WASD moves player, arrows move enemy, ESC quits.");

    Arc::Window window;
    if (!window.IsValid()) return 1;
    if (!Arc::GraphicsContext::Init()) return 1;
    Arc::Input::Init(window.NativeHandle());

    Arc::Renderer* renderer = Arc::CreateRenderer();
    renderer->SetViewport(0, 0, window.GetWidth(), window.GetHeight());
    Arc::RenderCommand::SetClearColor(Arc::Vec4(0.08f, 0.10f, 0.14f, 1.0f));

    Arc::Shader shader(kVert, kFrag);
    if (!shader.IsValid()) return 1;

    // --- Scene setup (Unity-like) ---
    Arc::Scene scene;
    float red[3] = {1, 0.2f, 0.2f};
    float blue[3] = {0.2f, 0.5f, 1.0f};

    Arc::Entity player = scene.CreateEntity("Player");
    player.AddComponent<Arc::MeshComponent>(Arc::MeshComponent::Triangle(red));
    player.GetComponent<Arc::TransformComponent>().Position = {-0.4f, 0.0f, 0.0f};

    Arc::Entity enemy = scene.CreateEntity("Enemy");
    enemy.AddComponent<Arc::MeshComponent>(Arc::MeshComponent::Triangle(blue));
    enemy.GetComponent<Arc::TransformComponent>().Position = {0.4f, 0.0f, 0.0f};
    enemy.GetComponent<Arc::TransformComponent>().Scale = {0.7f, 0.7f, 1.0f};

    Arc::Log::Info("entities: " + std::to_string(scene.EntityCount()));

    Arc::Time clock;
    while (!window.ShouldClose()) {
        clock.Tick();
        float dt = clock.Delta();
        float t = static_cast<float>(clock.Elapsed());

        auto& pt = player.GetComponent<Arc::TransformComponent>();
        if (Arc::Input::IsKeyDown(Arc::Key::A)) pt.Position.x -= 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::D)) pt.Position.x += 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::W)) pt.Position.y += 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::S)) pt.Position.y -= 1.2f * dt;

        auto& et = enemy.GetComponent<Arc::TransformComponent>();
        if (Arc::Input::IsKeyDown(Arc::Key::Left)) et.Position.x -= 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Right)) et.Position.x += 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Up)) et.Position.y += 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Down)) et.Position.y -= 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Escape)) break;

        // Idle spin for fun (Unity Update feel) — enemy slowly rotates
        et.Rotation.z = t * 0.8f;

        renderer->BeginFrame(Arc::RenderCommand::ClearColor());
        // Draw every entity with Transform + Mesh
        scene.Each<Arc::MeshComponent>([&](Arc::Entity e, Arc::MeshComponent& mesh) {
            mesh.Upload();
            const auto& tr = e.GetComponent<Arc::TransformComponent>();
            Arc::DrawMesh(mesh, shader, tr);
        });
        renderer->EndFrame();

        window.SwapBuffers();
        window.PollEvents();
        Arc::Input::EndFrame();
    }

    Arc::Input::Shutdown();
    Arc::Log::Info("05-scene: closed cleanly.");
    return 0;
}
