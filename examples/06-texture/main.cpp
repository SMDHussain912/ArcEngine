// M6b example: texture + audio together. Click/space plays blip, WASD moves.
#include "ArcEngine/Audio/Audio.h"
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Renderer/OpenGLRenderer.h"
#include "ArcEngine/Renderer/RenderCommand.h"
#include "ArcEngine/Renderer/Shader.h"
#include "ArcEngine/Renderer/Texture.h"
#include "ArcEngine/Scene/Components.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
const char* kVert = R"(
#version 460 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
out vec2 v_UV;
uniform mat4 u_Model;
void main() { v_UV = a_UV; gl_Position = u_Model * vec4(a_Pos, 1.0); }
)";
const char* kFrag = R"(
#version 460 core
in vec2 v_UV;
out vec4 FragColor;
uniform sampler2D u_Tex;
void main() { FragColor = texture(u_Tex, v_UV); }
)";
} // namespace

int main() {
    Arc::Log::Init();
    Arc::Log::Info("06-texture: Space/Click = blip sound, WASD = move, ESC = quit.");

    Arc::Window window;
    if (!window.IsValid()) return 1;
    if (!Arc::GraphicsContext::Init()) return 1;
    Arc::Input::Init(window.NativeHandle());
    Arc::Audio::Init(); // continues silent if no audio device

    Arc::Renderer* renderer = Arc::CreateRenderer();
    renderer->SetViewport(0, 0, window.GetWidth(), window.GetHeight());
    Arc::RenderCommand::SetClearColor(Arc::Vec4(0.08f, 0.10f, 0.14f, 1.0f));

    Arc::Texture tex("assets/textures/test.png");
    Arc::Shader shader(kVert, kFrag);
    if (!shader.IsValid()) return 1;
    shader.Bind();
    shader.SetInt("u_Tex", 0);

    float verts[] = {
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    };
    Arc::VertexArray vao;
    vao.Bind();
    Arc::VertexBuffer vbo(verts, sizeof(verts) / sizeof(float));
    vbo.Bind();
    vao.LayoutFloat(0, 3, 5 * sizeof(float), 0);
    vao.LayoutFloat(1, 2, 5 * sizeof(float), 3 * sizeof(float));
    vao.Unbind();

    Arc::TransformComponent tr;
    Arc::Time clock;

    while (!window.ShouldClose()) {
        clock.Tick();
        float dt = clock.Delta();

        if (Arc::Input::IsKeyDown(Arc::Key::A)) tr.Position.x -= 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::D)) tr.Position.x += 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::W)) tr.Position.y += 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::S)) tr.Position.y -= 1.2f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Escape)) break;

        if (Arc::Input::IsKeyPressed(Arc::Key::Space)) {
            Arc::Audio::Play("assets/audio/blip.wav");
            Arc::Log::Info("blip!");
        }
        if (Arc::Input::IsMousePressed(Arc::MouseButton::Left)) {
            Arc::Audio::Play("assets/audio/blip.wav", 0.7f);
        }

        renderer->BeginFrame(Arc::RenderCommand::ClearColor());
        shader.Bind();
        Arc::Mat4 m = tr.GetMatrix();
        shader.SetMat4("u_Model", &m[0][0]);
        tex.Bind(0);
        vao.Bind();
        renderer->DrawArrays(6);
        renderer->EndFrame();

        window.SwapBuffers();
        window.PollEvents();
        Arc::Input::EndFrame();
    }

    Arc::Audio::Shutdown();
    Arc::Input::Shutdown();
    Arc::Log::Info("06-texture: closed cleanly.");
    return 0;
}

