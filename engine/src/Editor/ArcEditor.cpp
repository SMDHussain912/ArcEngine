#include "ArcEngine/Editor/ArcEditor.h"
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Scene/MeshComponent.h"
#include "ArcEngine/Renderer/Shader.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstdint>

namespace Arc {

Scene* ArcEditor::s_scene = nullptr;

bool ArcEditor::Init(Window& window, Renderer* renderer, Scene& scene) {
    // ImGui context first (one per process; reuse the one the renderer may
    // own, but we own it here).
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Platform + renderer backends (reuse our already-bound GL context).
    // InitForOpenGL here is fine: it only registers callbacks; the GL
    // context must already be current when drawing. Renderer init pulls
    // its own GL loader (imgui_impl_opengl3_loader.h) — independent of glad.
    if (!ImGui_ImplGlfw_InitForOpenGL(window.NativeHandle(), true))
        return false;
    if (!ImGui_ImplOpenGL3_Init(nullptr)) // nullptr -> default GLSL 130
        return false;

    s_scene = &scene;

    Log::Info("ArcEditor: ImGui initialized (GLFW + OpenGL3 backend).");
    return true;
}

void ArcEditor::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    s_scene = nullptr;
}

void ArcEditor::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ArcEditor::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ArcEditor::HierarchyWindow() {
    if (!s_scene) return;
    ImGui::Begin("Hierarchy");
    for (Entity e : s_scene->Entities()) {
        if (!e.Valid()) continue;
        if (ImGui::Selectable(e.Name().c_str(), e == s_scene->GetFocusEntity())) {
            s_scene->SetFocusEntity(e);
        }
    }
    ImGui::End();
}

void ArcEditor::InspectorWindow() {
    if (!s_scene) return;
    ImGui::Begin("Inspector");

    Entity sel = s_scene->GetFocusEntity();
    if (sel.Valid()) {
        ImGui::Text("Selected: %s", sel.Name().c_str());
        if (s_scene->HasComponent<TransformComponent>(sel.Id())) {
            if (ImGui::TreeNode("Transform")) {
                auto& t = s_scene->GetComponent<TransformComponent>(sel.Id());
                ImGui::DragFloat3("Position", &t.Position.x);
                ImGui::DragFloat3("Rotation", &t.Rotation.x);
                ImGui::DragFloat3("Scale", &t.Scale.x);
                ImGui::TreePop();
            }
        }
        // Mesh stub (M7): per-entity material/mesh fields surface in a later
        // step with their own serialization pipeline.
        if (s_scene->HasComponent<MeshComponent>(sel.Id())) {
            if (ImGui::TreeNode("Mesh")) {
                auto& m = s_scene->GetComponent<MeshComponent>(sel.Id());
                ImGui::Text("Vertex count: %u", static_cast<unsigned>(m.Vertices.size() / 3));
                ImGui::TreePop();
            }
        }
    } else {
        ImGui::TextDisabled("No entity selected.");
    }
    ImGui::End();
}

void ArcEditor::ViewportWindow() {
    if (!s_scene) return;

    // Off-screen framebuffer (M7): render the live scene to a GL texture
    // (no window surface) so it can be sampled inside ImGui. Re-use a single
    // FBO + texture across frames; recreate if resized.
    static GLuint s_fbo = 0;
    static GLuint s_fboTex = 0;
    static int s_fboW = 0;
    static int s_fboH = 0;

    // Recreate when the window resized (viewport is 200x200).
    int w = 200, h = 200;
    if (s_fboW != w || s_fboH != h) {
        if (s_fbo) glDeleteFramebuffers(1, &s_fbo);
        if (s_fboTex) glDeleteTextures(1, &s_fboTex);
        glGenFramebuffers(1, &s_fbo);
        glGenTextures(1, &s_fboTex);
        glBindTexture(GL_TEXTURE_2D, s_fboTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(w),
                     static_cast<GLsizei>(h), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, s_fboTex, 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            char msg[128];
            std::snprintf(msg, sizeof(msg), "ArcEditor: off-screen FBO incomplete: 0x%08X", status);
            Log::Warn(msg);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        s_fboW = w;
        s_fboH = h;
    }

    // Minimal full-screen-ish colored quad shader (M7 viewport).
    //
    // Vertex:   a_Pos (3) + a_Color (3)  ->  gl_Position = vec4(a_Pos, 1)
    // Fragment: f_Color                ->  fragcolor
    static const char* s_vs = R"(
        #version 130
        in vec3 a_Pos;
        in vec3 a_Color;
        out vec3 f_Color;
        void main() {
            gl_Position = vec4(a_Pos, 1.0);
            f_Color = a_Color;
        }
    )";
    static const char* s_fs = R"(
        #version 130
        in vec3 f_Color;
        out vec4 fragColor;
        void main() { fragColor = vec4(f_Color, 1.0); }
    )";

    // Load/share the shader once (static local guarantees the full type is
    // known and the constructor runs at first call only).
    static Arc::Shader s_shader([]() -> Arc::Shader {
        Arc::Shader sh(s_vs, s_fs);
        return sh;
    }());

    glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw every entity in the scene. Each entity has a TransformComponent
    // (added by Scene::CreateEntity). Skip any entity without a MeshComponent.
    for (Entity e : s_scene->Entities()) {
        if (!e.Valid()) continue;
        if (!e.HasComponent<MeshComponent>()) continue;
        if (!e.GetComponent<MeshComponent>().Uploaded) continue;

        // Default transform (Scene::CreateEntity adds one by default).
        TransformComponent t;

        DrawMesh(e.GetComponent<MeshComponent>(), s_shader, t);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 1, 1); // back to the default tiny viewport

    // Show the FBO texture in the ImGui viewport.
    ImGui::Image((ImTextureID)(ImU64)(uintptr_t)s_fboTex, ImVec2(static_cast<float>(w), static_cast<float>(h)));
}

} // namespace Arc
