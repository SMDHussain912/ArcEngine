// M2 example: first real OpenGL 4.6 Core draw — an RGB triangle.
// Slow walkthrough:
//   Window (4.6 Core) -> GraphicsContext::Init (glad) -> Shader -> VAO+VBO -> glDrawArrays loop
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Time.h"
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/GraphicsContext.h"
#include "ArcEngine/Renderer/Shader.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <filesystem>
#include <string>
#include <vector>

namespace {
std::string FindAsset(const std::string& rel) {
    // Try: cwd, project root guesses, binary dir parents. Keeps example runnable
    // from project root AND from build/examples/02-triangle/.
    namespace fs = std::filesystem;
    std::vector<fs::path> bases = { fs::current_path() };
    fs::path exe = fs::current_path();
    // walk up to 4 levels from cwd looking for assets/
    fs::path p = fs::current_path();
    for (int i = 0; i < 5; i++) {
        bases.push_back(p);
        if (p.has_parent_path()) p = p.parent_path();
    }
    // also relative to this source file location at runtime is unknown, so check common spots
    for (auto& b : bases) {
        fs::path cand = b / rel;
        if (fs::exists(cand)) return cand.string();
    }
    return rel; // let Shader report the error
}
} // namespace

int main() {
    Arc::Log::Init();
    Arc::Log::Info("02-triangle: RGB triangle on OpenGL 4.6 Core. ESC to close.");

    Arc::Window window; // defaults: 1280x720, 4.6 Core
    if (!window.IsValid()) return 1;

    if (!Arc::GraphicsContext::Init()) return 1;

    // Triangle: position (xyz) + color (rgb) interleaved
    float vertices[] = {
        // pos              // color
         0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // top, red
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // left, green
         0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f, // right, blue
    };

    Arc::Shader shader = Arc::Shader::FromFiles(
        FindAsset("assets/shaders/triangle.vert"), FindAsset("assets/shaders/triangle.frag"));
    // Fallback: also try from build dir (cmake copies? no — we run from project root).
    // If file load failed (empty program), try relative-to-binary path is skipped for slowness.
    if (!shader.IsValid()) {
        Arc::Log::Error("Shader invalid. Run from project root so assets/shaders/ resolves.");
        return 1;
    }

    Arc::VertexArray vao;
    vao.Bind();
    Arc::VertexBuffer vbo(vertices, sizeof(vertices) / sizeof(float));
    vbo.Bind();
    // stride = 6 floats (pos+color), offsets 0 and 3 floats
    vao.LayoutFloat(0, 3, 6 * sizeof(float), 0);
    vao.LayoutFloat(1, 3, 6 * sizeof(float), 3 * sizeof(float));
    vao.Unbind();

    Arc::Time clock;
    while (!window.ShouldClose()) {
        clock.Tick();

        GLFWwindow* native = window.NativeHandle();
        if (glfwGetKey(native, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(native, GLFW_TRUE);

        glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Bind();
        vao.Bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.SwapBuffers();
        window.PollEvents();
    }

    Arc::Log::Info("02-triangle: closed cleanly.");
    return 0;
}
