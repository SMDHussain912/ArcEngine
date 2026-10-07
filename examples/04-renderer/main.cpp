// M4 example: same triangle as M2/M3 but NO raw glClear/glDrawArrays in game code.
// Game code talks only to: Renderer + RenderCommand + Shader + Buffer.
// Swap OpenGLRenderer -> OpenGLESRenderer later (M8) with zero changes here.
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Renderer/OpenGLRenderer.h"
#include "ArcEngine/Renderer/RenderCommand.h"
#include "ArcEngine/Renderer/Shader.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
const char* kVert = R"(
#version 460 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Color;
out vec3 v_Color;
uniform vec2 u_Offset;
void main() {
    v_Color = a_Color;
    gl_Position = vec4(a_Pos.xy + u_Offset, a_Pos.z, 1.0);
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
    Arc::Log::Info("04-renderer: triangle via Renderer abstraction. WASD moves, ESC quits.");

    Arc::Window window;
    if (!window.IsValid()) return 1;
    if (!Arc::GraphicsContext::Init()) return 1;
    Arc::Input::Init(window.NativeHandle());

    Arc::Renderer* renderer = Arc::CreateRenderer();
    renderer->SetViewport(0, 0, window.GetWidth(), window.GetHeight());
    Arc::RenderCommand::SetClearColor(Arc::Vec4(0.08f, 0.10f, 0.14f, 1.0f));

    float vertices[] = {
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
    };
    Arc::Shader shader(kVert, kFrag);
    if (!shader.IsValid()) return 1;

    Arc::VertexArray vao;
    vao.Bind();
    Arc::VertexBuffer vbo(vertices, sizeof(vertices) / sizeof(float));
    vbo.Bind();
    vao.LayoutFloat(0, 3, 6 * sizeof(float), 0);
    vao.LayoutFloat(1, 3, 6 * sizeof(float), 3 * sizeof(float));
    vao.Unbind();

    Arc::Vec2 offset(0, 0);
    Arc::Time clock;

    while (!window.ShouldClose()) {
        clock.Tick();
        float dt = clock.Delta();

        if (Arc::Input::IsKeyDown(Arc::Key::A)) offset.x -= 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::D)) offset.x += 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::W)) offset.y += 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::S)) offset.y -= 1.5f * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Escape)) break;

        renderer->BeginFrame(Arc::RenderCommand::ClearColor());
        shader.Bind();
        shader.SetVec2("u_Offset", offset.x, offset.y);
        vao.Bind();
        renderer->DrawArrays(3);
        renderer->EndFrame();

        window.SwapBuffers();
        window.PollEvents();
        Arc::Input::EndFrame();
    }

    Arc::Input::Shutdown();
    Arc::Log::Info("04-renderer: closed cleanly.");
    return 0;
}
