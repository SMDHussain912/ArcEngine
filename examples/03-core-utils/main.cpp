// M3 example: Math + Input + File working together.
// WASD/arrows move the triangle via u_Offset uniform, Space toggles bg, click logs mouse.
#include "ArcEngine/Core/File.h"
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
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
uniform int u_Mode;
void main() {
    vec3 c = (u_Mode == 0) ? v_Color : vec3(1.0) - v_Color;
    FragColor = vec4(c, 1.0);
}
)";
} // namespace

int main() {
    Arc::Log::Init();

    // --- File demo: read back our own shader, print size ---
    std::string vertSrc = Arc::File::ReadText("assets/shaders/triangle.vert");
    Arc::Log::Info("triangle.vert bytes: " + std::to_string(vertSrc.size()));
    Arc::Log::Info(std::string("shader exists: ") +
                   (Arc::File::Exists("assets/shaders/triangle.frag") ? "yes" : "no"));

    // --- Math demo: build a model matrix, print one element ---
    Arc::Mat4 model(1.0f);
    model = Arc::Translate(model, Arc::Vec3(0.2f, 0.0f, 0.0f));
    model = Arc::Rotate(model, Arc::Radians(15.0f), Arc::Vec3(0, 0, 1));
    Arc::Log::Info("model[3][0] (tx) = " + std::to_string(model[3][0]));

    Arc::Window window;
    if (!window.IsValid()) return 1;
    if (!Arc::GraphicsContext::Init()) return 1;
    Arc::Input::Init(window.NativeHandle());

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

    Arc::Vec2 offset(0.0f, 0.0f);
    Arc::Time clock;
    int colorMode = 0;
    Arc::Log::Info("WASD/arrows = move, Space = invert colors, Click = log mouse, ESC = quit.");

    while (!window.ShouldClose()) {
        clock.Tick();
        float dt = clock.Delta();
        float speed = 1.5f;

        if (Arc::Input::IsKeyDown(Arc::Key::A) || Arc::Input::IsKeyDown(Arc::Key::Left))
            offset.x -= speed * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::D) || Arc::Input::IsKeyDown(Arc::Key::Right))
            offset.x += speed * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::W) || Arc::Input::IsKeyDown(Arc::Key::Up))
            offset.y += speed * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::S) || Arc::Input::IsKeyDown(Arc::Key::Down))
            offset.y -= speed * dt;
        if (Arc::Input::IsKeyDown(Arc::Key::Escape)) break;

        if (Arc::Input::IsKeyPressed(Arc::Key::Space)) {
            colorMode = (colorMode + 1) % 2;
            Arc::Log::Info("color mode: " + std::to_string(colorMode));
        }
        if (Arc::Input::IsMousePressed(Arc::MouseButton::Left)) {
            Arc::Vec2 m = Arc::Input::MousePos();
            Arc::Log::Info("click at " + std::to_string((int)m.x) + "," + std::to_string((int)m.y));
        }

        glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Bind();
        shader.SetVec2("u_Offset", offset.x, offset.y);
        shader.SetInt("u_Mode", colorMode);
        vao.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.SwapBuffers();
        window.PollEvents();
        Arc::Input::EndFrame();
    }

    Arc::Input::Shutdown();
    Arc::Log::Info("03-core-utils: closed cleanly.");
    return 0;
}

