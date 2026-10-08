#include "ArcEngine/Editor/ArcEditor.h"
#include "ArcEngine/Core/Input.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Scene/MeshComponent.h"
#include "ArcEngine/Scene/SceneSerializer.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/Shader.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace Arc {

namespace {
constexpr float kGizmoLen = 0.55f; // world units at zoom 1
constexpr size_t kUndoCap = 100;
constexpr size_t kConsoleCap = 200;

float SnapVal(float v, float snap) {
    if (snap <= 0.0f) return v;
    return std::round(v / snap) * snap;
}
} // namespace

bool ArcEditor::Init(Window& window, Renderer* renderer, Scene& scene) {
    if (m_initialized) {
        SetScene(scene);
        return true;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // no imgui.ini sidecar; fixed panel layout

    if (!ImGui_ImplGlfw_InitForOpenGL(window.NativeHandle(), true)) return false;
    if (!ImGui_ImplOpenGL3_Init("#version 460")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    m_window = &window;
    m_renderer = renderer;
    m_scene = &scene;
    m_initialized = true;
    ApplyTheme();
    ResetCamera();
    AddLog("ArcEditor initialized (GLFW + OpenGL3, GLSL 460).");
    Log::Info("ArcEditor: ImGui initialized (GLFW + OpenGL3 backend).");
    return true;
}

void ArcEditor::Shutdown() {
    if (!m_initialized) return;
    DestroyGL();
    m_sprites.ReleaseGL(); // while the GL context is still current
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
    m_scene = nullptr;
    m_window = nullptr;
    m_renderer = nullptr;
    m_initialized = false;
}

void ArcEditor::SetScene(Scene& scene) {
    m_scene = &scene;
    m_undo.clear();
    m_redo.clear();
    m_dragging = false;
    m_panning = false;
    m_inspectEditing = false;
    ResetCamera();
}

void ArcEditor::BeginFrame(float dt) {
    if (!m_initialized) return;
    m_frames++;
    m_viewFps = m_viewFps * 0.95f + (dt > 0.0f ? 1.0f / dt : 60.0f) * 0.05f;
    if (m_statusMsgTime > 0.0f) m_statusMsgTime -= dt;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ArcEditor::EndFrame() {
    if (!m_initialized) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ArcEditor::DrawAll() {
    // Fixed engine layout (vendored ImGui has no docking branch): place every
    // panel explicitly each frame — menu 24px, toolbar 36px, status 24px,
    // hierarchy left, inspector right, viewport center, console bottom.
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 origin = vp->Pos;
    ImVec2 size = vp->Size;
    m_layoutOrigin = origin;
    m_layoutSize = size;
    DrawMenuBar();
    DrawToolbar();
    HierarchyWindow();
    InspectorWindow();
    ViewportWindow();
    ConsoleWindow();
    StatusBar();
}

void ArcEditor::ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.FrameRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.WindowRounding = 6.0f;
    s.ChildRounding = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.FramePadding = ImVec2(6, 4);
    s.ItemSpacing = ImVec2(6, 4);
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.13f, 1.0f);
    c[ImGuiCol_ChildBg] = ImVec4(0.07f, 0.08f, 0.11f, 1.0f);
    c[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.13f, 0.17f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.13f, 0.17f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.18f, 0.24f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.20f, 0.32f, 0.55f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.40f, 0.65f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.30f, 0.52f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.32f, 0.55f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.44f, 0.70f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.28f, 0.50f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.15f, 0.20f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.21f, 0.28f, 1.0f);
}

void ArcEditor::Play() {
    if (!m_scene || m_play == PlayState::Playing) return;
    m_playBackup.Clear();
    for (Entity e : m_scene->Entities()) {
        Entity n = m_playBackup.CreateEntity(e.Name());
        if (e.HasComponent<TransformComponent>())
            n.GetComponent<TransformComponent>() = e.GetComponent<TransformComponent>();
    }
    m_hasPlayBackup = true;
    m_play = PlayState::Playing;
    AddLog("Play mode started (entity transforms backed up).");
}

void ArcEditor::Pause() {
    if (m_play == PlayState::Playing) {
        m_play = PlayState::Paused;
        AddLog("Play mode paused.");
    } else if (m_play == PlayState::Paused) {
        m_play = PlayState::Playing;
        AddLog("Play mode resumed.");
    }
}

void ArcEditor::Stop() {
    if (!m_scene || m_play == PlayState::Edit) return;
    if (m_hasPlayBackup) {
        // Restore edit-time transforms from the backup (Godot-style: play never
        // mutates the edited scene).
        for (Entity b : m_playBackup.Entities()) {
            for (Entity e : m_scene->Entities()) {
                if (e.Name() == b.Name() && e.HasComponent<TransformComponent>() &&
                    b.HasComponent<TransformComponent>()) {
                    e.GetComponent<TransformComponent>() = b.GetComponent<TransformComponent>();
                    break;
                }
            }
        }
        m_playBackup.Clear();
        m_hasPlayBackup = false;
    }
    m_play = PlayState::Edit;
    AddLog("Stopped. Edit-time scene restored.");
}

bool ArcEditor::SaveScene(const std::string& path) {
    if (!m_scene) return false;
    if (SceneSerializer::Save(*m_scene, path)) {
        m_dirty = false;
        std::snprintf(m_statusMsg, sizeof(m_statusMsg), "Saved %s", path.c_str());
        m_statusMsgTime = 3.0f;
        AddLog(std::string("Saved scene to ") + path);
        return true;
    }
    AddLog(std::string("Save FAILED: ") + path);
    return false;
}

bool ArcEditor::LoadScene(const std::string& path) {
    if (!m_scene) return false;
    Scene tmp;
    if (!SceneSerializer::Load(path, tmp)) {
        AddLog(std::string("Load FAILED: ") + path);
        return false;
    }
    m_scene->Clear();
    for (Entity e : tmp.Entities()) {
        Entity n = m_scene->CreateEntity(e.Name());
        if (e.HasComponent<TransformComponent>())
            n.GetComponent<TransformComponent>() = e.GetComponent<TransformComponent>();
        if (e.HasComponent<MeshComponent>()) {
            MeshComponent m = e.GetComponent<MeshComponent>();
            m.Uploaded = false;
            m.Vao.reset();
            m.Vbo.reset();
            MeshComponent& d = n.AddComponent<MeshComponent>();
            d = std::move(m);
            d.Upload();
        }
        if (e.HasComponent<CameraComponent>())
            n.AddComponent<CameraComponent>() = e.GetComponent<CameraComponent>();
        if (e.HasComponent<LightComponent>())
            n.AddComponent<LightComponent>() = e.GetComponent<LightComponent>();
        if (e.HasComponent<GuiComponent>())
            n.AddComponent<GuiComponent>() = e.GetComponent<GuiComponent>();
        if (e.HasComponent<SpriteComponent>())
            n.AddComponent<SpriteComponent>() = e.GetComponent<SpriteComponent>();
    }
    m_scene->SetFocusEntity(Entity());
    m_undo.clear();
    m_redo.clear();
    m_dirty = false;
    std::snprintf(m_statusMsg, sizeof(m_statusMsg), "Loaded %s", path.c_str());
    m_statusMsgTime = 3.0f;
    AddLog(std::string("Loaded scene from ") + path);
    return true;
}

void ArcEditor::AddLog(const std::string& msg) {
    m_console.push_back(msg);
    if (m_console.size() > kConsoleCap) m_console.erase(m_console.begin());
    Log::Info("Editor: " + msg);
}

void ArcEditor::ResetCamera() {
    m_camCenter = Vec2(0.0f, 0.0f);
    m_camZoom = 1.0f;
}

Mat4 ArcEditor::ViewProj(int w, int h) const {
    float aspect = h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;
    float halfH = 1.0f / m_camZoom;
    float halfW = halfH * aspect;
    Mat4 proj = Ortho(m_camCenter.x - halfW, m_camCenter.x + halfW,
                       m_camCenter.y - halfH, m_camCenter.y + halfH, -10.0f, 10.0f);
    return proj;
}

ImVec2 ArcEditor::WorldToScreen(const Vec3& world, const ImVec2& imgMin, const ImVec2& imgMax,
                                int w, int h) const {
    Mat4 vp = ViewProj(w, h);
    Vec4 clip = vp * Vec4(world, 1.0f);
    float ndcX = clip.x; // ortho: w == 1
    float ndcY = clip.y;
    float sx = imgMin.x + (ndcX * 0.5f + 0.5f) * (imgMax.x - imgMin.x);
    float sy = imgMin.y + (0.5f - ndcY * 0.5f) * (imgMax.y - imgMin.y);
    return ImVec2(sx, sy);
}

Vec3 ArcEditor::ScreenToWorld(float mx, float my, const ImVec2& imgMin, const ImVec2& imgMax,
                              int w, int h) const {
    float u = (imgMax.x > imgMin.x) ? (mx - imgMin.x) / (imgMax.x - imgMin.x) : 0.5f;
    float v = (imgMax.y > imgMin.y) ? (my - imgMin.y) / (imgMax.y - imgMin.y) : 0.5f;
    float ndcX = u * 2.0f - 1.0f;
    float ndcY = 1.0f - v * 2.0f;
    float aspect = h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;
    float halfH = 1.0f / m_camZoom;
    float halfW = halfH * aspect;
    return Vec3(m_camCenter.x + ndcX * halfW, m_camCenter.y + ndcY * halfH, 0.0f);
}

void ArcEditor::PushUndo(uint32_t id, const TransformComponent& before,
                        const TransformComponent& after) {
    m_undo.push_back(UndoEntry{id, before, after});
    if (m_undo.size() > kUndoCap) m_undo.erase(m_undo.begin());
    m_redo.clear();
}

void ArcEditor::Undo() {
    if (!m_scene || m_undo.empty()) return;
    UndoEntry e = m_undo.back();
    m_undo.pop_back();
    if (m_scene->HasComponent<TransformComponent>(e.entityId)) {
        auto& t = m_scene->GetComponent<TransformComponent>(e.entityId);
        m_redo.push_back(UndoEntry{e.entityId, t, e.before});
        t = e.before;
        AddLog("Undo entity " + std::to_string(e.entityId));
    }
}

void ArcEditor::Redo() {
    if (!m_scene || m_redo.empty()) return;
    UndoEntry e = m_redo.back();
    m_redo.pop_back();
    if (m_scene->HasComponent<TransformComponent>(e.entityId)) {
        auto& t = m_scene->GetComponent<TransformComponent>(e.entityId);
        m_undo.push_back(UndoEntry{e.entityId, t, e.after});
        t = e.after;
        AddLog("Redo entity " + std::to_string(e.entityId));
    }
}

Entity ArcEditor::DuplicateEntity(Entity src) {
    if (!m_scene || !src.Valid()) return Entity();
    Entity dst = m_scene->CreateEntity(src.Name() + " Copy");
    if (src.HasComponent<TransformComponent>())
        dst.GetComponent<TransformComponent>() = src.GetComponent<TransformComponent>();
    if (src.HasComponent<MeshComponent>()) {
        MeshComponent m = src.GetComponent<MeshComponent>();
        m.Uploaded = false;
        m.Vao.reset();
        m.Vbo.reset();
        MeshComponent& d = dst.AddComponent<MeshComponent>();
        d = std::move(m);
        d.Upload();
    }
    if (src.HasComponent<CameraComponent>())
        dst.AddComponent<CameraComponent>() = src.GetComponent<CameraComponent>();
    if (src.HasComponent<LightComponent>())
        dst.AddComponent<LightComponent>() = src.GetComponent<LightComponent>();
    if (src.HasComponent<GuiComponent>())
        dst.AddComponent<GuiComponent>() = src.GetComponent<GuiComponent>();
    if (src.HasComponent<SpriteComponent>())
        dst.AddComponent<SpriteComponent>() = src.GetComponent<SpriteComponent>();
    m_scene->SetFocusEntity(dst);
    m_dirty = true;
    AddLog("Duplicated '" + src.Name() + "' as '" + dst.Name() + "'");
    return dst;
}

bool ArcEditor::EntityAtScreen(const ImVec2& imgMin, const ImVec2& imgMax, int w, int h,
                              float mx, float my, Entity& out) {
    if (!m_scene) return false;
    float best = 24.0f;
    bool found = false;
    for (Entity e : m_scene->Entities()) {
        if (!e.Valid() || !e.HasComponent<TransformComponent>()) continue;
        const auto& t = e.GetComponent<TransformComponent>();
        ImVec2 p = WorldToScreen(t.Position, imgMin, imgMax, w, h);
        float d = std::hypot(p.x - mx, p.y - my);
        float radius = 16.0f;
        if (e.HasComponent<MeshComponent>()) {
            const auto& m = e.GetComponent<MeshComponent>();
            float ext = 0.0f;
            for (size_t i = 0; i + 2 < m.Vertices.size(); i += 6) {
                Vec4 wp = t.GetMatrix() * Vec4(m.Vertices[i], m.Vertices[i + 1], 0.0f, 1.0f);
                ImVec2 sp = WorldToScreen(Vec3(wp.x, wp.y, 0.0f), imgMin, imgMax, w, h);
                ext = std::max(ext, std::hypot(sp.x - p.x, sp.y - p.y));
            }
            radius = std::max(16.0f, ext + 8.0f);
        }
        if (d < radius && d < best) {
            best = d;
            out = e;
            found = true;
        }
    }
    return found;
}

void ArcEditor::EnsureGL() {
    if (m_glReady) return;
    static const char* kSceneVs = R"GLSL(#version 460 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec3 a_Color;
out vec3 v_Color;
uniform mat4 u_ViewProj;
uniform mat4 u_Model;
void main() {
    v_Color = a_Color;
    gl_Position = u_ViewProj * u_Model * vec4(a_Pos, 1.0);
}
)GLSL";
    static const char* kSceneFs = R"GLSL(#version 460 core
in vec3 v_Color;
out vec4 FragColor;
void main() { FragColor = vec4(v_Color, 1.0); }
)GLSL";
    static const char* kFlatVs = R"GLSL(#version 460 core
layout(location = 0) in vec2 a_Pos;
uniform mat4 u_ViewProj;
void main() { gl_Position = u_ViewProj * vec4(a_Pos, 0.0, 1.0); }
)GLSL";
    static const char* kFlatFs = R"GLSL(#version 460 core
uniform vec3 u_Color;
out vec4 FragColor;
void main() { FragColor = vec4(u_Color, 1.0); }
)GLSL";
    m_sceneShader = std::make_unique<Shader>(kSceneVs, kSceneFs);
    m_flatShader = std::make_unique<Shader>(kFlatVs, kFlatFs);
    if (!m_sceneShader->IsValid() || !m_flatShader->IsValid()) {
        Log::Error("ArcEditor: viewport shaders failed to compile.");
        m_sceneShader.reset();
        m_flatShader.reset();
        return;
    }
    std::vector<float> grid;
    for (int i = -10; i <= 10; ++i) {
        float f = static_cast<float>(i);
        grid.push_back(f);
        grid.push_back(-10.0f);
        grid.push_back(f);
        grid.push_back(10.0f);
        grid.push_back(-10.0f);
        grid.push_back(f);
        grid.push_back(10.0f);
        grid.push_back(f);
    }
    m_gridVerts = static_cast<int>(grid.size() / 2);
    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, grid.size() * sizeof(float), grid.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
    glBindVertexArray(0);
    m_gridVao = vao;
    m_gridVbo = vbo;
    m_glReady = true;
}

void ArcEditor::DestroyGL() {
    GLuint vbo = m_gridVbo, vao = m_gridVao, fbo = m_fbo, tex = m_fboTex;
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    if (tex) glDeleteTextures(1, &tex);
    m_gridVao = m_gridVbo = m_fbo = m_fboTex = 0;
    m_gridVerts = 0;
    m_fboW = m_fboH = 0;
    m_sceneShader.reset();
    m_flatShader.reset();
    m_glReady = false;
}

void ArcEditor::RenderSceneToFBO(int w, int h) {
    EnsureGL();
    if (!m_glReady || !m_sceneShader || !m_flatShader) return;
    if (w <= 0 || h <= 0) return;

    if (m_fbo == 0 || m_fboTex == 0 || m_fboW != w || m_fboH != h) {
        GLuint oldFbo = m_fbo, oldTex = m_fboTex;
        if (oldFbo) glDeleteFramebuffers(1, &oldFbo);
        if (oldTex) glDeleteTextures(1, &oldTex);
        m_fbo = m_fboTex = 0;
        GLuint fbo = 0, tex = 0;
        glGenFramebuffers(1, &fbo);
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(w),
                     static_cast<GLsizei>(h), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            char msg[128];
            std::snprintf(msg, sizeof(msg), "ArcEditor: off-screen FBO incomplete: 0x%08X",
                          status);
            Log::Error(msg);
            glDeleteFramebuffers(1, &fbo);
            glDeleteTextures(1, &tex);
            return;
        }
        m_fbo = fbo;
        m_fboTex = tex;
        m_fboW = w;
        m_fboH = h;
    }

    GLint prevFbo = 0, prevVp[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    glGetIntegerv(GL_VIEWPORT, prevVp);
    GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
    glDisable(GL_DEPTH_TEST);
    glClearColor(0.07f, 0.08f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Mat4 vp = ViewProj(w, h);
    if (m_showGrid && m_gridVao) {
        m_flatShader->Bind();
        m_flatShader->SetMat4("u_ViewProj", &vp[0][0]);
        m_flatShader->SetVec3("u_Color", 0.22f, 0.24f, 0.30f);
        glBindVertexArray(m_gridVao);
        glDrawArrays(GL_LINES, 0, m_gridVerts);
        glBindVertexArray(0);
    }

    // P1: sprite entities batch here — one indexed draw per texture bind.
    m_sprites.Draw(*m_scene, vp);

    Entity focus = m_scene->GetFocusEntity();
    m_sceneShader->Bind();
    m_sceneShader->SetMat4("u_ViewProj", &vp[0][0]);
    for (Entity e : m_scene->Entities()) {
        if (!e.Valid()) continue;
        const TransformComponent& t =
            e.HasComponent<TransformComponent>()
                ? static_cast<const TransformComponent&>(e.GetComponent<TransformComponent>())
                : TransformComponent{};
        if (e.HasComponent<MeshComponent>()) {
            MeshComponent& mesh = e.GetComponent<MeshComponent>();
            mesh.Upload();
            if (!mesh.Uploaded || !mesh.Vao) continue;
            Mat4 m = t.GetMatrix();
            m_sceneShader->SetMat4("u_Model", &m[0][0]);
            mesh.Vao->Bind();
            uint32_t count = static_cast<uint32_t>(mesh.Vertices.size() / 6);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(count));
        }
        // Camera frustum: zoom-box outline so cameras are visible/selectable.
        if (e.HasComponent<CameraComponent>()) {
            const auto& c = e.GetComponent<CameraComponent>();
            float hz = 1.0f / std::max(0.05f, c.Zoom);
            float ar = w > 0 && h > 0 ? static_cast<float>(w) / static_cast<float>(h) : 1.6f;
            float hx = hz * ar, hy = hz;
            float x0 = t.Position.x - hx, x1 = t.Position.x + hx;
            float y0 = t.Position.y - hy, y1 = t.Position.y + hy;
            float box[8] = {x0, y0, x1, y0, x1, y1, x0, y1};
            GLuint bv = 0, ba = 0;
            glGenVertexArrays(1, &ba);
            glGenBuffers(1, &bv);
            glBindVertexArray(ba);
            glBindBuffer(GL_ARRAY_BUFFER, bv);
            glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
            m_flatShader->Bind();
            m_flatShader->SetMat4("u_ViewProj", &vp[0][0]);
            if (c.Active)
                m_flatShader->SetVec3("u_Color", 0.35f, 0.75f, 1.0f);
            else
                m_flatShader->SetVec3("u_Color", 0.35f, 0.4f, 0.5f);
            glDrawArrays(GL_LINE_LOOP, 0, 4);
            glBindVertexArray(0);
            glDeleteBuffers(1, &bv);
            glDeleteVertexArrays(1, &ba);
            m_sceneShader->Bind();
        }
        // Light radius ring.
        if (e.HasComponent<LightComponent>()) {
            const auto& l = e.GetComponent<LightComponent>();
            const int segs = 48;
            std::vector<float> ring;
            ring.reserve((segs + 1) * 2);
            for (int i = 0; i <= segs; ++i) {
                float a = 2.0f * 3.14159265f * static_cast<float>(i) / segs;
                ring.push_back(t.Position.x + std::cos(a) * l.Radius);
                ring.push_back(t.Position.y + std::sin(a) * l.Radius);
            }
            GLuint rv = 0, ra = 0;
            glGenVertexArrays(1, &ra);
            glGenBuffers(1, &rv);
            glBindVertexArray(ra);
            glBindBuffer(GL_ARRAY_BUFFER, rv);
            glBufferData(GL_ARRAY_BUFFER, ring.size() * sizeof(float), ring.data(),
                         GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
            m_flatShader->Bind();
            m_flatShader->SetMat4("u_ViewProj", &vp[0][0]);
            m_flatShader->SetVec3("u_Color", l.Color.x * l.Intensity,
                                  l.Color.y * l.Intensity, l.Color.z * l.Intensity);
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<int>(ring.size() / 2));
            glBindVertexArray(0);
            glDeleteBuffers(1, &rv);
            glDeleteVertexArrays(1, &ra);
            m_sceneShader->Bind();
        }
        // GUI overlay quad (screen-aligned at the entity position).
        if (e.HasComponent<GuiComponent>()) {
            const auto& g = e.GetComponent<GuiComponent>();
            float hx = g.Size.x * 0.5f * t.Scale.x, hy = g.Size.y * 0.5f * t.Scale.y;
            float x0 = t.Position.x - hx, x1 = t.Position.x + hx;
            float y0 = t.Position.y - hy, y1 = t.Position.y + hy;
            float quad[12] = {x0, y0, x1, y0, x1, y1, x0, y0, x1, y1, x0, y1};
            GLuint qv = 0, qa = 0;
            glGenVertexArrays(1, &qa);
            glGenBuffers(1, &qv);
            glBindVertexArray(qa);
            glBindBuffer(GL_ARRAY_BUFFER, qv);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
            m_flatShader->Bind();
            m_flatShader->SetMat4("u_ViewProj", &vp[0][0]);
            m_flatShader->SetVec3("u_Color", g.BgColor.x, g.BgColor.y, g.BgColor.z);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            glDeleteBuffers(1, &qv);
            glDeleteVertexArrays(1, &qa);
            m_sceneShader->Bind();
        }
    }

    if (focus.Valid() && focus.HasComponent<TransformComponent>()) {
        const auto& t = focus.GetComponent<TransformComponent>();
        float r = 0.06f / m_camZoom + 0.02f;
        std::vector<float> ring;
        const int segs = 40;
        for (int i = 0; i <= segs; ++i) {
            float a = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segs);
            ring.push_back(t.Position.x + std::cos(a) * r);
            ring.push_back(t.Position.y + std::sin(a) * r);
        }
        GLuint rv = 0, ra = 0;
        glGenVertexArrays(1, &ra);
        glGenBuffers(1, &rv);
        glBindVertexArray(ra);
        glBindBuffer(GL_ARRAY_BUFFER, rv);
        glBufferData(GL_ARRAY_BUFFER, ring.size() * sizeof(float), ring.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
        m_flatShader->Bind();
        m_flatShader->SetMat4("u_ViewProj", &vp[0][0]);
        m_flatShader->SetVec3("u_Color", 1.0f, 0.8f, 0.2f);
        glDrawArrays(GL_LINE_STRIP, 0, static_cast<int>(ring.size() / 2));
        glBindVertexArray(0);
        glDeleteBuffers(1, &rv);
        glDeleteVertexArrays(1, &ra);
    }

    glBindVertexArray(0);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
    if (prevDepth) glEnable(GL_DEPTH_TEST);
}

void ArcEditor::UpdateGizmoDrag(const ImVec2& imgMin, const ImVec2& imgMax, int w, int h) {
    if (!m_scene || !m_dragging) return;
    Entity sel = m_scene->GetFocusEntity();
    if (!sel.Valid() || !sel.HasComponent<TransformComponent>()) {
        m_dragging = false;
        return;
    }
    Vec2 mp = Input::MousePos();
    if (!Input::IsMouseDown(MouseButton::Left)) {
        auto& t = sel.GetComponent<TransformComponent>();
        PushUndo(sel.Id(), m_dragBefore, t);
        m_dragging = false;
        m_dragAxis = 0;
        m_dirty = true;
        return;
    }
    Vec3 cur = ScreenToWorld(mp.x, mp.y, imgMin, imgMax, w, h);
    auto& t = sel.GetComponent<TransformComponent>();
    if (m_gizmo == GizmoMode::Translate) {
        Vec3 d{cur.x - m_dragWorldStart.x, cur.y - m_dragWorldStart.y, 0.0f};
        Vec3 base = m_dragBefore.Position;
        if (m_dragAxis == 1)
            t.Position = Vec3(SnapVal(base.x + d.x, m_snap), base.y, base.z);
        else if (m_dragAxis == 2)
            t.Position = Vec3(base.x, SnapVal(base.y + d.y, m_snap), base.z);
        else
            t.Position = Vec3(SnapVal(base.x + d.x, m_snap), SnapVal(base.y + d.y, m_snap), 0.0f);
    } else if (m_gizmo == GizmoMode::Scale) {
        Vec3 o = m_dragBefore.Position;
        Vec3 s0{std::max(0.01f, std::fabs(o.x - m_dragWorldStart.x)),
                std::max(0.01f, std::fabs(o.y - m_dragWorldStart.y)), 1.0f};
        float fx = (m_dragAxis == 2) ? 1.0f : std::fabs(cur.x - o.x) / s0.x;
        float fy = (m_dragAxis == 1) ? 1.0f : std::fabs(cur.y - o.y) / s0.y;
        fx = std::max(0.05f, fx);
        fy = std::max(0.05f, fy);
        t.Scale = Vec3(m_dragBefore.Scale.x * fx, m_dragBefore.Scale.y * fy,
                       m_dragBefore.Scale.z);
    } else {
        Vec3 o = m_dragBefore.Position;
        float a0 = std::atan2(m_dragWorldStart.y - o.y, m_dragWorldStart.x - o.x);
        float a1 = std::atan2(cur.y - o.y, cur.x - o.x);
        t.Rotation.z = m_dragBefore.Rotation.z + (a1 - a0);
    }
}

void ArcEditor::UpdateViewportInput(const ImVec2& imgMin, const ImVec2& imgMax, int w, int h) {
    if (!m_scene) return;
    ImGuiIO& io = ImGui::GetIO();
    Vec2 mp = Input::MousePos();
    // Gate on the viewport image being hovered (not WantCaptureMouse: the Image
    // itself captures the mouse every frame, which starved the old gate and made
    // gizmo clicks dead — Godot routes picking on the unhandled-input pass).
    bool hovered = ImGui::IsItemHovered();
    bool inside = hovered ||
                  (mp.x >= imgMin.x && mp.x <= imgMax.x && mp.y >= imgMin.y && mp.y <= imgMax.y);

    // Wheel zoom (only when hovering the image and not scrolling another widget).
    if (hovered && io.MouseWheel != 0.0f) {
        Vec3 before = ScreenToWorld(mp.x, mp.y, imgMin, imgMax, w, h);
        m_camZoom = std::max(0.1f, std::min(32.0f, m_camZoom * (1.0f + io.MouseWheel * 0.1f)));
        Vec3 after = ScreenToWorld(mp.x, mp.y, imgMin, imgMax, w, h);
        m_camCenter.x += before.x - after.x;
        m_camCenter.y += before.y - after.y;
    }

    // Middle-drag (or Space+Left) pans; ImGui already consumed left for image hover.
    bool panBtn = Input::IsMouseDown(MouseButton::Middle) ||
                  (Input::IsMouseDown(MouseButton::Left) &&
                   Input::IsKeyDown(Key::Space));
    if (panBtn && inside) {
        if (!m_panning) {
            m_panning = true;
            m_panLast = mp;
        } else {
            float dxPx = mp.x - m_panLast.x;
            float dyPx = mp.y - m_panLast.y;
            float worldPerPxX = (2.0f / m_camZoom) *
                                (static_cast<float>(w) / static_cast<float>(h)) /
                                (imgMax.x - imgMin.x);
            float worldPerPxY = (2.0f / m_camZoom) / (imgMax.y - imgMin.y);
            m_camCenter.x -= dxPx * worldPerPxX;
            m_camCenter.y += dyPx * worldPerPxY;
            m_panLast = mp;
        }
    } else {
        m_panning = false;
    }

    if (m_dragging) {
        UpdateGizmoDrag(imgMin, imgMax, w, h);
        return;
    }

    // Left-click: gizmo handle wins, else click-select.
    if (inside && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !Input::IsKeyDown(Key::Space)) {
        Entity sel = m_scene->GetFocusEntity();
        int axis = 0;
        if (sel.Valid() && sel.HasComponent<TransformComponent>()) {
            const auto& t = sel.GetComponent<TransformComponent>();
            ImVec2 o = WorldToScreen(t.Position, imgMin, imgMax, w, h);
            float len = kGizmoLen * m_camZoom * (imgMax.x - imgMin.x) /
                        (2.0f * (static_cast<float>(w) / static_cast<float>(h)));
            len = std::max(28.0f, std::min(120.0f, len));
            ImVec2 ex{o.x + len, o.y}, ey{o.x, o.y - len};
            if (std::hypot(mp.x - ex.x, mp.y - ex.y) < 12.0f) axis = 1;
            if (std::hypot(mp.x - ey.x, mp.y - ey.y) < 12.0f) axis = 2;
        }
        if (axis != 0) {
            m_dragAxis = axis;
            m_dragging = true;
            m_dragBefore = sel.GetComponent<TransformComponent>();
            m_dragWorldStart = ScreenToWorld(mp.x, mp.y, imgMin, imgMax, w, h);
        } else {
            Entity hit;
            if (EntityAtScreen(imgMin, imgMax, w, h, mp.x, mp.y, hit)) {
                m_scene->SetFocusEntity(hit);
                AddLog("Selected '" + hit.Name() + "'");
            } else {
                m_scene->SetFocusEntity(Entity());
            }
        }
    }
}

void ArcEditor::DrawMenuBar() {
    if (!m_scene) return;
    if (!ImGui::BeginMainMenuBar()) return;
    if (ImGui::BeginMenu("File")) {
        ImGui::InputText("Save##path", m_savePath, sizeof(m_savePath));
        ImGui::InputText("Load##path", m_loadPath, sizeof(m_loadPath));
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
            if (SaveScene(m_savePath))
                std::strncpy(m_loadPath, m_savePath, sizeof(m_loadPath) - 1);
        }
        if (ImGui::MenuItem("Load Scene", "Ctrl+O")) LoadScene(m_loadPath);
        ImGui::Separator();
        if (ImGui::MenuItem("New Scene (clear)")) {
            m_scene->Clear();
            m_scene->SetFocusEntity(Entity());
            m_undo.clear();
            m_redo.clear();
            m_dirty = false;
            ResetCamera();
            AddLog("New empty scene.");
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
        bool canUndo = !m_undo.empty();
        bool canRedo = !m_redo.empty();
        if (ImGui::MenuItem("Undo", "Ctrl+Z", false, canUndo)) Undo();
        if (ImGui::MenuItem("Redo", "Ctrl+Y", false, canRedo)) Redo();
        ImGui::Separator();
        if (ImGui::MenuItem("Duplicate Selected", "Ctrl+D", false,
                            m_scene->GetFocusEntity().Valid())) {
            DuplicateEntity(m_scene->GetFocusEntity());
        }
        if (ImGui::MenuItem("Delete Selected", "Del", false,
                            m_scene->GetFocusEntity().Valid())) {
            Entity sel = m_scene->GetFocusEntity();
            AddLog("Deleted '" + sel.Name() + "'");
            m_scene->DestroyEntity(sel);
            m_dirty = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Entity")) {
        if (ImGui::MenuItem("Create Empty", "Ctrl+Shift+N")) {
            Entity e = m_scene->CreateEntity("Entity");
            m_scene->SetFocusEntity(e);
            m_renamingId = e.Id();
            std::strncpy(m_rename, e.Name().c_str(), sizeof(m_rename) - 1);
            m_dirty = true;
            AddLog("Created entity '" + e.Name() + "'");
        }
        if (ImGui::BeginMenu("Create Mesh")) {
            const char* ids[] = {"Triangle", "Quad",    "Circle", "Ring",
                                 "Plane",    "Cross",   "Arrow"};
            for (const char* label : ids) {
                if (ImGui::MenuItem(label)) {
                    std::string id = label;
                    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
                    float white[3] = {0.9f, 0.9f, 0.9f};
                    Entity e = m_scene->CreateEntity(label);
                    MeshComponent m = MeshComponent::FromId(id, white);
                    m.MeshId = id;
                    MeshComponent& d = e.AddComponent<MeshComponent>();
                    d = std::move(m);
                    d.Upload();
                    m_scene->SetFocusEntity(e);
                    m_dirty = true;
                    AddLog(std::string("Created ") + label + " entity.");
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem("Create Camera")) {
            Entity e = m_scene->CreateEntity("Camera");
            e.AddComponent<CameraComponent>();
            m_scene->SetFocusEntity(e);
            m_dirty = true;
            AddLog("Created camera entity.");
        }
        if (ImGui::MenuItem("Create Sprite")) {
            Entity e = m_scene->CreateEntity("Sprite");
            SpriteComponent& sp = e.AddComponent<SpriteComponent>();
            sp.TexturePath = "assets/textures/test.png";
            m_scene->SetFocusEntity(e);
            m_dirty = true;
            AddLog("Created sprite entity.");
        }
        if (ImGui::MenuItem("Create Light")) {
            Entity e = m_scene->CreateEntity("Light");
            e.AddComponent<LightComponent>();
            m_scene->SetFocusEntity(e);
            m_dirty = true;
            AddLog("Created light entity.");
        }
        if (ImGui::BeginMenu("Create GUI")) {
            const char* kinds[] = {"Button", "Label", "Panel"};
            for (int i = 0; i < 3; ++i) {
                if (ImGui::MenuItem(kinds[i])) {
                    Entity e = m_scene->CreateEntity(kinds[i]);
                    GuiComponent& g = e.AddComponent<GuiComponent>();
                    g.Widget = static_cast<GuiComponent::Kind>(i);
                    std::snprintf(g.Text, sizeof(g.Text), "%s", kinds[i]);
                    m_scene->SetFocusEntity(e);
                    m_dirty = true;
                    AddLog(std::string("Created GUI ") + kinds[i] + ".");
                }
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Load Starter Scene")) {
            m_scene->Clear();
            float ground[3] = {0.25f, 0.28f, 0.33f};
            Entity g = m_scene->CreateEntity("Ground");
            MeshComponent gm = MeshComponent::Plane(ground, 8.0f);
            gm.MeshId = "plane";
            MeshComponent& gd = g.AddComponent<MeshComponent>();
            gd = std::move(gm);
            gd.Upload();
            g.GetComponent<TransformComponent>().Position = {0.0f, -1.2f, 0.0f};
            float red[3] = {0.9f, 0.3f, 0.3f};
            Entity p = m_scene->CreateEntity("Player");
            MeshComponent pm = MeshComponent::Triangle(red);
            pm.MeshId = "triangle";
            MeshComponent& pd = p.AddComponent<MeshComponent>();
            pd = std::move(pm);
            pd.Upload();
            float blue[3] = {0.3f, 0.55f, 0.95f};
            Entity box = m_scene->CreateEntity("Box");
            MeshComponent bm = MeshComponent::Quad(blue);
            bm.MeshId = "quad";
            MeshComponent& bd = box.AddComponent<MeshComponent>();
            bd = std::move(bm);
            bd.Upload();
            box.GetComponent<TransformComponent>().Position = {1.2f, -0.4f, 0.0f};
            Entity cam = m_scene->CreateEntity("MainCamera");
            cam.AddComponent<CameraComponent>();
            Entity sun = m_scene->CreateEntity("Sun");
            sun.GetComponent<TransformComponent>().Position = {-1.5f, 1.0f, 0.0f};
            sun.AddComponent<LightComponent>();
            Entity btn = m_scene->CreateEntity("PlayButton");
            btn.GetComponent<TransformComponent>().Position = {0.0f, 1.2f, 0.0f};
            btn.AddComponent<GuiComponent>();
            m_scene->SetFocusEntity(p);
            m_undo.clear();
            m_redo.clear();
            m_dirty = false;
            ResetCamera();
            AddLog("Starter scene loaded (ground, player, box, camera, light, GUI).");
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Grid", "G", &m_showGrid);
        if (ImGui::MenuItem("Reset Camera", "Home")) {
            ResetCamera();
            AddLog("Camera reset.");
        }
        ImGui::Separator();
        int mode = static_cast<int>(m_gizmo);
        if (ImGui::MenuItem("Move Gizmo", "W", mode == 0)) m_gizmo = GizmoMode::Translate;
        if (ImGui::MenuItem("Rotate Gizmo", "E", mode == 1)) m_gizmo = GizmoMode::Rotate;
        if (ImGui::MenuItem("Scale Gizmo", "R", mode == 2)) m_gizmo = GizmoMode::Scale;
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Play")) {
        if (ImGui::MenuItem("Play", "F5", false, m_play == PlayState::Edit)) Play();
        if (ImGui::MenuItem(m_play == PlayState::Paused ? "Resume" : "Pause", "F6", false,
                            m_play != PlayState::Edit))
            Pause();
        if (ImGui::MenuItem("Stop", "Shift+F5", false, m_play != PlayState::Edit)) Stop();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();

    ImGuiIO& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
        if (SaveScene(m_savePath)) std::strncpy(m_loadPath, m_savePath, sizeof(m_loadPath) - 1);
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_O)) LoadScene(m_loadPath);
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_D) && m_scene->GetFocusEntity().Valid())
        DuplicateEntity(m_scene->GetFocusEntity());
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        if (m_play == PlayState::Edit)
            Play();
        else
            Stop();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F6)) Pause();
    if (ImGui::IsKeyPressed(ImGuiKey_W) && !ImGui::GetIO().WantTextInput)
        m_gizmo = GizmoMode::Translate;
    if (ImGui::IsKeyPressed(ImGuiKey_E) && !ImGui::GetIO().WantTextInput)
        m_gizmo = GizmoMode::Rotate;
    if (ImGui::IsKeyPressed(ImGuiKey_R) && !ImGui::GetIO().WantTextInput)
        m_gizmo = GizmoMode::Scale;
    if (ImGui::IsKeyPressed(ImGuiKey_G)) m_showGrid = !m_showGrid;
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) ResetCamera();
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_scene->GetFocusEntity().Valid() &&
        !ImGui::GetIO().WantTextInput) {
        Entity sel = m_scene->GetFocusEntity();
        AddLog("Deleted '" + sel.Name() + "'");
        m_scene->DestroyEntity(sel);
        m_dirty = true;
    }
}

void ArcEditor::DrawToolbar() {
    float y = m_layoutOrigin.y + 24.0f;
    ImGui::SetNextWindowPos(ImVec2(m_layoutOrigin.x, y));
    ImGui::SetNextWindowSize(ImVec2(m_layoutSize.x, 36.0f));
    ImGui::Begin("Toolbar", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    if (m_play == PlayState::Edit) {
        if (ImGui::Button("|> Play (F5)")) Play();
    } else {
        if (ImGui::Button("[] Stop")) Stop();
        ImGui::SameLine();
        if (ImGui::Button(m_play == PlayState::Paused ? ">> Resume" : "|| Pause")) Pause();
    }
    ImGui::SameLine();
    ImVec4 tint = m_play == PlayState::Playing
                      ? ImVec4(0.3f, 0.9f, 0.4f, 1.0f)
                      : (m_play == PlayState::Paused ? ImVec4(0.95f, 0.8f, 0.3f, 1.0f)
                                                     : ImVec4(0.6f, 0.65f, 0.75f, 1.0f));
    ImGui::TextColored(tint, "%s",
                       m_play == PlayState::Playing
                           ? "PLAYING"
                           : (m_play == PlayState::Paused ? "PAUSED" : "EDIT"));
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    int mode = static_cast<int>(m_gizmo);
    ImGui::RadioButton("Move (W)", &mode, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Rotate (E)", &mode, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Scale (R)", &mode, 2);
    m_gizmo = static_cast<GizmoMode>(mode);
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();
    ImGui::Checkbox("Grid (G)", &m_showGrid);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(90);
    ImGui::DragFloat("Snap", &m_snap, 0.01f, 0.0f, 10.0f, "%.2f");
    ImGui::SameLine();
    ImGui::TextDisabled("MMB/Space-drag pan, wheel zoom, click select");
    ImGui::End();
}

void ArcEditor::HierarchyWindow() {
    if (!m_scene) return;
    float w = 250.0f;
    float y = m_layoutOrigin.y + 60.0f;
    float h = m_layoutSize.y - 60.0f - 170.0f - 24.0f;
    ImGui::SetNextWindowPos(ImVec2(m_layoutOrigin.x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
    ImGui::TextDisabled("%d entities%s", static_cast<int>(m_scene->EntityCount()),
                        m_dirty ? " *" : "");
    if (ImGui::Button("+ Entity")) {
        Entity e = m_scene->CreateEntity("Entity");
        m_scene->SetFocusEntity(e);
        m_renamingId = e.Id();
        std::strncpy(m_rename, e.Name().c_str(), sizeof(m_rename) - 1);
        m_dirty = true;
        AddLog("Created entity '" + e.Name() + "'");
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##search", "Search...", m_search, sizeof(m_search));

    Entity focus = m_scene->GetFocusEntity();
    std::string filter = m_search;
    std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);
    int shown = 0;
    for (Entity e : m_scene->Entities()) {
        if (!e.Valid()) continue;
        if (!filter.empty()) {
            std::string n = e.Name();
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (n.find(filter) == std::string::npos) continue;
        }
        ++shown;
        ImGui::PushID(static_cast<int>(e.Id()));
        bool sel = focus.Valid() && focus.Id() == e.Id();
        const char* icon = e.HasComponent<MeshComponent>()
                               ? "[#] "
                               : (e.HasComponent<SpriteComponent>() ? "[s] " : "[ ] ");
        std::string label = std::string(icon) + e.Name();
        if (ImGui::Selectable(label.c_str(), sel)) m_scene->SetFocusEntity(e);
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            m_renamingId = e.Id();
            std::strncpy(m_rename, e.Name().c_str(), sizeof(m_rename) - 1);
        }
        if (ImGui::BeginPopupContextItem("entity_ctx")) {
            if (ImGui::MenuItem("Rename")) {
                m_renamingId = e.Id();
                std::strncpy(m_rename, e.Name().c_str(), sizeof(m_rename) - 1);
            }
            if (ImGui::MenuItem("Duplicate")) DuplicateEntity(e);
            if (ImGui::BeginMenu("Add Mesh")) {
                const char* ids[] = {"triangle", "quad",    "circle", "ring",
                                     "plane",    "cross",   "arrow"};
                for (const char* id : ids) {
                    if (ImGui::MenuItem(id) && !e.HasComponent<MeshComponent>()) {
                        float white[3] = {0.9f, 0.9f, 0.9f};
                        MeshComponent m = MeshComponent::FromId(id, white);
                        m.MeshId = id;
                        MeshComponent& d = e.AddComponent<MeshComponent>();
                        d = std::move(m);
                        d.Upload();
                        m_dirty = true;
                        AddLog(std::string("Added ") + id + " to '" + e.Name() + "'");
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Add Camera") && !e.HasComponent<CameraComponent>()) {
                e.AddComponent<CameraComponent>();
                m_dirty = true;
                AddLog("Added camera to '" + e.Name() + "'");
            }
            if (ImGui::MenuItem("Add Light") && !e.HasComponent<LightComponent>()) {
                e.AddComponent<LightComponent>();
                m_dirty = true;
                AddLog("Added light to '" + e.Name() + "'");
            }
            if (ImGui::MenuItem("Add GUI") && !e.HasComponent<GuiComponent>()) {
                e.AddComponent<GuiComponent>();
                m_dirty = true;
                AddLog("Added GUI to '" + e.Name() + "'");
            }
            if (ImGui::MenuItem("Delete")) {
                AddLog("Deleted '" + e.Name() + "'");
                m_scene->DestroyEntity(e);
                m_dirty = true;
                if (m_renamingId == e.Id()) m_renamingId = 0;
                ImGui::EndPopup();
                ImGui::PopID();
                continue;
            }
            ImGui::EndPopup();
        }
        if (m_renamingId == e.Id()) {
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##rename", m_rename, sizeof(m_rename),
                                 ImGuiInputTextFlags_EnterReturnsTrue |
                                     ImGuiInputTextFlags_AutoSelectAll)) {
                m_scene->RenameEntity(e.Id(), m_rename);
                m_dirty = true;
                AddLog(std::string("Renamed to '") + m_rename + "'");
                m_renamingId = 0;
            }
            if (!ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape)) m_renamingId = 0;
        }
        ImGui::PopID();
    }
    if (shown == 0) ImGui::TextDisabled("No entities match.");
    ImGui::End();
}


void ArcEditor::InspectorWindow() {
    if (!m_scene) return;
    float w = 300.0f;
    float y = m_layoutOrigin.y + 60.0f;
    float h = m_layoutSize.y - 60.0f - 170.0f - 24.0f;
    ImGui::SetNextWindowPos(ImVec2(m_layoutOrigin.x + m_layoutSize.x - w, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("Inspector", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
    Entity sel = m_scene->GetFocusEntity();
    if (!sel.Valid()) {
        ImGui::TextDisabled("No entity selected.");
        ImGui::End();
        m_inspectEditing = false;
        return;
    }
    if (m_play != PlayState::Edit) ImGui::BeginDisabled();
    ImGui::Text("Selected: %s  (id %u)", sel.Name().c_str(), sel.Id());

    if (sel.HasComponent<TransformComponent>()) {
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& t = sel.GetComponent<TransformComponent>();
            if (!m_inspectEditing || m_inspectId != sel.Id()) {
                m_inspectBefore = t;
                m_inspectId = sel.Id();
            }
            bool active = false;
            ImGui::DragFloat3("Position", &t.Position.x, 0.01f);
            active = active || ImGui::IsItemActive();
            float rotDeg[3] = {Degrees(t.Rotation.x), Degrees(t.Rotation.y),
                               Degrees(t.Rotation.z)};
            if (ImGui::DragFloat3("Rotation (deg)", rotDeg, 0.5f)) {
                t.Rotation = Vec3(Radians(rotDeg[0]), Radians(rotDeg[1]), Radians(rotDeg[2]));
            }
            active = active || ImGui::IsItemActive();
            ImGui::DragFloat3("Scale", &t.Scale.x, 0.01f, 0.01f, 100.0f);
            active = active || ImGui::IsItemActive();
            if (active) {
                m_inspectEditing = true;
            } else if (m_inspectEditing && m_inspectId == sel.Id()) {
                PushUndo(sel.Id(), m_inspectBefore, t);
                m_inspectEditing = false;
                m_dirty = true;
            }
            if (ImGui::Button("Reset Transform")) {
                PushUndo(sel.Id(), t, TransformComponent{});
                t = TransformComponent{};
                m_dirty = true;
            }
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Button("Add TransformComponent")) {
            sel.AddComponent<TransformComponent>();
            m_dirty = true;
            AddLog("Added Transform to '" + sel.Name() + "'");
        }
    }

    if (sel.HasComponent<MeshComponent>()) {
        if (ImGui::TreeNodeEx("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& m = sel.GetComponent<MeshComponent>();
            ImGui::Text("Vertex count: %u", static_cast<unsigned>(m.Vertices.size() / 6));
            ImGui::Text("Uploaded: %s", m.Uploaded ? "yes" : "no");
            float tint[3] = {1.0f, 1.0f, 1.0f};
            size_t n = 0;
            for (size_t i = 0; i + 5 < m.Vertices.size(); i += 6) {
                tint[0] += m.Vertices[i + 3];
                tint[1] += m.Vertices[i + 4];
                tint[2] += m.Vertices[i + 5];
                ++n;
            }
            if (n > 0) {
                tint[0] /= static_cast<float>(n);
                tint[1] /= static_cast<float>(n);
                tint[2] /= static_cast<float>(n);
            }
            if (ImGui::ColorEdit3("Tint", tint)) {
                for (size_t i = 0; i + 5 < m.Vertices.size(); i += 6) {
                    m.Vertices[i + 3] = tint[0];
                    m.Vertices[i + 4] = tint[1];
                    m.Vertices[i + 5] = tint[2];
                }
                m.Uploaded = false;
                m.Vao.reset();
                m.Vbo.reset();
                m.Upload();
                m_dirty = true;
            }
            if (ImGui::Button("Remove Mesh")) {
                m.Uploaded = false;
                m.Vao.reset();
                m.Vbo.reset();
                sel.RemoveComponent<MeshComponent>();
                m_dirty = true;
                AddLog("Removed mesh from '" + sel.Name() + "'");
            }
            ImGui::Separator();
            ImGui::TextDisabled("Primitive: %s", m.MeshId.c_str());
            if (ImGui::BeginCombo("##prim", m.MeshId.c_str())) {
                const char* ids[] = {"triangle", "quad",    "circle", "ring",
                                     "plane",    "cross",   "arrow"};
                for (const char* id : ids) {
                    bool cur = m.MeshId == id;
                    if (ImGui::Selectable(id, cur) && !cur) {
                        float rgb[3] = {0.9f, 0.9f, 0.9f};
                        size_t n = 0;
                        float ar = 0, ag = 0, ab = 0;
                        for (size_t i = 0; i + 5 < m.Vertices.size(); i += 6) {
                            ar += m.Vertices[i + 3];
                            ag += m.Vertices[i + 4];
                            ab += m.Vertices[i + 5];
                            ++n;
                        }
                        if (n > 0) {
                            rgb[0] = ar / n;
                            rgb[1] = ag / n;
                            rgb[2] = ab / n;
                        }
                        MeshComponent nm = MeshComponent::FromId(id, rgb);
                        nm.MeshId = id;
                        m = std::move(nm);
                        m.Upload();
                        m_dirty = true;
                        AddLog(std::string("Switched to ") + id);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Button("Add Triangle Mesh")) {
            float white[3] = {0.9f, 0.9f, 0.9f};
            sel.AddComponent<MeshComponent>(MeshComponent::Triangle(white));
            sel.GetComponent<MeshComponent>().Upload();
            m_dirty = true;
            AddLog("Added triangle to '" + sel.Name() + "'");
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Quad Mesh")) {
            float white[3] = {0.9f, 0.9f, 0.9f};
            sel.AddComponent<MeshComponent>(MeshComponent::Quad(white));
            sel.GetComponent<MeshComponent>().Upload();
            m_dirty = true;
            AddLog("Added quad to '" + sel.Name() + "'");
        }
    }
    if (sel.HasComponent<SpriteComponent>()) {
        if (ImGui::TreeNodeEx("Sprite", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& sp = sel.GetComponent<SpriteComponent>();
            char pathBuf[256];
            std::snprintf(pathBuf, sizeof(pathBuf), "%s", sp.TexturePath.c_str());
            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputTextWithHint("Texture##tex", "assets/textures/...", pathBuf,
                                         sizeof(pathBuf))) {
                sp.TexturePath = pathBuf;
                m_dirty = true;
            }
            float tint[4] = {sp.Tint.x, sp.Tint.y, sp.Tint.z, sp.Tint.w};
            if (ImGui::ColorEdit4("Tint", tint)) {
                sp.Tint = Vec4(tint[0], tint[1], tint[2], tint[3]);
                m_dirty = true;
            }
            ImGui::SetNextItemWidth(-1);
            if (ImGui::DragFloat2("Size", &sp.Size.x, 0.01f, 0.01f, 100.0f, "%.2f"))
                m_dirty = true;
            int layer = sp.Layer;
            ImGui::SetNextItemWidth(80);
            if (ImGui::DragInt("Layer", &layer, 1.0f, -100, 100)) {
                sp.Layer = layer;
                m_dirty = true;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("FlipX", &sp.FlipX)) m_dirty = true;
            ImGui::SameLine();
            if (ImGui::Checkbox("FlipY", &sp.FlipY)) m_dirty = true;
            if (ImGui::TreeNode("UV Region")) {
                ImGui::SetNextItemWidth(-1);
                if (ImGui::DragFloat2("Min", &sp.RegionMin.x, 0.01f, 0.0f, 1.0f, "%.3f"))
                    m_dirty = true;
                ImGui::SetNextItemWidth(-1);
                if (ImGui::DragFloat2("Max", &sp.RegionMax.x, 0.01f, 0.0f, 1.0f, "%.3f"))
                    m_dirty = true;
                if (ImGui::Button("Reset Region")) {
                    sp.RegionMin = Vec2(0.0f, 0.0f);
                    sp.RegionMax = Vec2(1.0f, 1.0f);
                    m_dirty = true;
                }
                ImGui::TreePop();
            }
            if (ImGui::Checkbox("Linear filter", &sp.FilterLinear)) m_dirty = true;
            if (ImGui::Button("Remove Sprite")) {
                sel.RemoveComponent<SpriteComponent>();
                m_dirty = true;
                AddLog("Removed sprite from '" + sel.Name() + "'");
            }
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Button("Add Sprite")) {
            SpriteComponent& sp = sel.AddComponent<SpriteComponent>();
            sp.TexturePath = "assets/textures/test.png";
            m_dirty = true;
            AddLog("Added sprite to '" + sel.Name() + "'");
        }
    }
    if (sel.HasComponent<CameraComponent>()) {
        if (ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& c = sel.GetComponent<CameraComponent>();
            if (ImGui::Checkbox("Active", &c.Active)) m_dirty = true;
            if (ImGui::DragFloat("Zoom", &c.Zoom, 0.01f, 0.05f, 32.0f, "%.2f")) {
                c.Zoom = std::max(0.05f, c.Zoom);
                m_dirty = true;
            }
            if (ImGui::Button("Remove Camera")) {
                sel.RemoveComponent<CameraComponent>();
                m_dirty = true;
                AddLog("Removed camera from '" + sel.Name() + "'");
            }
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Button("Add Camera")) {
            sel.AddComponent<CameraComponent>();
            m_dirty = true;
            AddLog("Added camera to '" + sel.Name() + "'");
        }
    }
    if (sel.HasComponent<LightComponent>()) {
        if (ImGui::TreeNodeEx("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& l = sel.GetComponent<LightComponent>();
            float col[3] = {l.Color.x, l.Color.y, l.Color.z};
            if (ImGui::ColorEdit3("Color", col)) {
                l.Color = Vec3(col[0], col[1], col[2]);
                m_dirty = true;
            }
            if (ImGui::DragFloat("Intensity", &l.Intensity, 0.01f, 0.0f, 8.0f, "%.2f"))
                m_dirty = true;
            if (ImGui::DragFloat("Radius", &l.Radius, 0.01f, 0.05f, 50.0f, "%.2f")) {
                l.Radius = std::max(0.05f, l.Radius);
                m_dirty = true;
            }
            if (ImGui::Button("Remove Light")) {
                sel.RemoveComponent<LightComponent>();
                m_dirty = true;
                AddLog("Removed light from '" + sel.Name() + "'");
            }
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Button("Add Light")) {
            sel.AddComponent<LightComponent>();
            m_dirty = true;
            AddLog("Added light to '" + sel.Name() + "'");
        }
    }
    if (sel.HasComponent<GuiComponent>()) {
        if (ImGui::TreeNodeEx("GUI", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& g = sel.GetComponent<GuiComponent>();
            int kind = static_cast<int>(g.Widget);
            if (ImGui::Combo("Widget", &kind, "Button\0Label\0Panel\0")) {
                g.Widget = static_cast<GuiComponent::Kind>(kind);
                m_dirty = true;
            }
            if (ImGui::DragFloat2("Size", &g.Size.x, 0.01f, 0.05f, 20.0f, "%.2f"))
                m_dirty = true;
            if (ImGui::InputText("Text", g.Text, sizeof(g.Text))) m_dirty = true;
            float bg[3] = {g.BgColor.x, g.BgColor.y, g.BgColor.z};
            if (ImGui::ColorEdit3("BG", bg)) {
                g.BgColor = Vec3(bg[0], bg[1], bg[2]);
                m_dirty = true;
            }
            float fg[3] = {g.TextColor.x, g.TextColor.y, g.TextColor.z};
            if (ImGui::ColorEdit3("Text color", fg)) {
                g.TextColor = Vec3(fg[0], fg[1], fg[2]);
                m_dirty = true;
            }
            if (ImGui::Button("Remove GUI")) {
                sel.RemoveComponent<GuiComponent>();
                m_dirty = true;
                AddLog("Removed GUI from '" + sel.Name() + "'");
            }
            ImGui::TreePop();
        }
    } else {
        if (ImGui::Button("Add GUI")) {
            sel.AddComponent<GuiComponent>();
            m_dirty = true;
            AddLog("Added GUI to '" + sel.Name() + "'");
        }
    }
    if (m_play != PlayState::Edit) ImGui::EndDisabled();
    ImGui::End();
}



void ArcEditor::DrawGizmoOverlay(const ImVec2& imgMin, const ImVec2& imgMax) {
    if (!m_scene) return;
    Entity sel = m_scene->GetFocusEntity();
    if (!sel.Valid() || !sel.HasComponent<TransformComponent>()) return;
    const auto& t = sel.GetComponent<TransformComponent>();
    ImVec2 o = WorldToScreen(t.Position, imgMin, imgMax, m_fboW, m_fboH);
    float aspect = m_fboH > 0 ? static_cast<float>(m_fboW) / static_cast<float>(m_fboH) : 1.0f;
    float len = kGizmoLen * m_camZoom * (imgMax.x - imgMin.x) / (2.0f * aspect);
    len = std::max(28.0f, std::min(120.0f, len));
    ImVec2 ex{o.x + len, o.y}, ey{o.x, o.y - len};
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float rotR = 0.0f;
    if (m_gizmo == GizmoMode::Rotate) {
        float r = len * 0.8f;
        rotR = r;
        dl->AddCircle(o, r, IM_COL32(250, 200, 60, 255), 48, 2.0f);
        dl->AddText(ImVec2(o.x + r + 4, o.y - 8), IM_COL32(250, 200, 60, 255), "R");
    } else if (m_gizmo == GizmoMode::Scale) {
        dl->AddLine(o, ex, IM_COL32(230, 90, 90, 255), 3.0f);
        dl->AddLine(o, ey, IM_COL32(110, 220, 110, 255), 3.0f);
        dl->AddRectFilled(ImVec2(ex.x - 5, ex.y - 5), ImVec2(ex.x + 5, ex.y + 5),
                          IM_COL32(230, 90, 90, 255));
        dl->AddRectFilled(ImVec2(ey.x - 5, ey.y - 5), ImVec2(ey.x + 5, ey.y + 5),
                          IM_COL32(110, 220, 110, 255));
    } else {
        dl->AddLine(o, ex, IM_COL32(230, 90, 90, 255), 3.0f);
        dl->AddLine(o, ey, IM_COL32(110, 220, 110, 255), 3.0f);
        dl->AddTriangleFilled(ImVec2(ex.x + 8, ex.y), ImVec2(ex.x - 2, ex.y - 5),
                              ImVec2(ex.x - 2, ex.y + 5), IM_COL32(230, 90, 90, 255));
        dl->AddTriangleFilled(ImVec2(ey.x, ey.y - 8), ImVec2(ey.x - 5, ey.y + 2),
                              ImVec2(ey.x + 5, ey.y + 2), IM_COL32(110, 220, 110, 255));
    }
    dl->AddCircleFilled(o, 4.0f, IM_COL32(240, 240, 240, 255));
    if (m_dragging) {
        dl->AddText(ImVec2(o.x + 8, o.y + 8), IM_COL32(255, 255, 255, 255),
                    m_dragAxis == 1 ? "X" : (m_dragAxis == 2 ? "Y" : "..."));
    }
    if (rotR > 0.0f) {
        Vec2 mp = Input::MousePos();
        dl->AddLine(o, ImVec2(mp.x, mp.y), IM_COL32(250, 200, 60, 160), 1.0f);
    }
}

void ArcEditor::ViewportWindow() {
    if (!m_scene) return;
    float x = m_layoutOrigin.x + 250.0f;
    float y = m_layoutOrigin.y + 60.0f;
    float wPanel = m_layoutSize.x - 250.0f - 300.0f;
    float hPanel = m_layoutSize.y - 60.0f - 170.0f - 24.0f;
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(wPanel, hPanel));
    ImGui::Begin("Viewport", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
    Entity sel = m_scene->GetFocusEntity();
    ImGui::TextDisabled("Ent: %d  Cam(%.2f, %.2f) Zoom %.2fx  %.0f fps",
                        static_cast<int>(m_scene->EntityCount()), m_camCenter.x, m_camCenter.y,
                        m_camZoom, m_viewFps);
    ImGui::SameLine();
    if (ImGui::SmallButton("Reset Cam")) ResetCamera();
    ImGui::SameLine();
    const char* gizmoName = m_gizmo == GizmoMode::Translate
                                ? "Move"
                                : (m_gizmo == GizmoMode::Rotate ? "Rotate" : "Scale");
    ImGui::TextDisabled("| %s", gizmoName);
    if (m_play != PlayState::Edit) {
        ImGui::SameLine();
        ImGui::TextColored(m_play == PlayState::Playing ? ImVec4(0.3f, 0.9f, 0.4f, 1.0f)
                                                        : ImVec4(0.95f, 0.8f, 0.3f, 1.0f),
                           "%s", m_play == PlayState::Playing ? "[PLAYING]" : "[PAUSED]");
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    int w = std::max(64, static_cast<int>(avail.x));
    int h = std::max(64, static_cast<int>(avail.y - 4));
    h = std::min(h, w);
    w = h;

    RenderSceneToFBO(w, h);
    if (m_fboTex == 0) {
        ImGui::TextDisabled("Viewport unavailable (FBO init failed).");
        ImGui::End();
        return;
    }
    ImVec2 imgMin = ImGui::GetCursorScreenPos();
    ImGui::Image(static_cast<ImTextureID>(static_cast<uintptr_t>(m_fboTex)),
                 ImVec2(static_cast<float>(w), static_cast<float>(h)), ImVec2(0, 1),
                 ImVec2(1, 0));
    ImVec2 imgMax{imgMin.x + static_cast<float>(w), imgMin.y + static_cast<float>(h)};
    UpdateViewportInput(imgMin, imgMax, w, h);
    DrawGizmoOverlay(imgMin, imgMax);
    if (sel.Valid()) {
        ImGui::TextDisabled("Sel: %s | %s gizmo | drag handle", sel.Name().c_str(),
                            m_gizmo == GizmoMode::Translate
                                ? "Move"
                                : (m_gizmo == GizmoMode::Rotate ? "Rotate" : "Scale"));
    }
    ImGui::End();
}

void ArcEditor::ConsoleWindow() {
    float w = m_layoutSize.x - 250.0f - 300.0f;
    float h = 170.0f;
    float x = m_layoutOrigin.x + 250.0f;
    float y = m_layoutOrigin.y + m_layoutSize.y - h - 24.0f;
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("Console", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
    if (ImGui::SmallButton("Clear")) m_console.clear();
    ImGui::SameLine();
    ImGui::TextDisabled("%d lines", static_cast<int>(m_console.size()));
    ImGui::BeginChild("##log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    for (const auto& line : m_console) ImGui::TextUnformatted(line.c_str());
    if (!m_console.empty()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();
}

void ArcEditor::StatusBar() {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 pos{vp->Pos.x, vp->Pos.y + vp->Size.y - 24.0f};
    ImVec2 size{vp->Size.x, 24.0f};
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::Begin("##statusbar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);
    const char* msg = (m_statusMsgTime > 0.0f && m_statusMsg[0] != '\0') ? m_statusMsg : "Ready";
    ImGui::TextDisabled("Ent %d | Sel %s | %s%s | Cam(%.2f, %.2f) x%.2f | %.0f fps | %s",
                        static_cast<int>(m_scene ? m_scene->EntityCount() : 0),
                        (m_scene && m_scene->GetFocusEntity().Valid())
                            ? m_scene->GetFocusEntity().Name().c_str()
                            : "-",
                        m_play == PlayState::Playing
                            ? "Playing"
                            : (m_play == PlayState::Paused ? "Paused" : "Edit"),
                        (m_dirty && m_play == PlayState::Edit) ? "*" : "", m_camCenter.x,
                        m_camCenter.y, m_camZoom, m_viewFps, msg);
    ImGui::End();
}
} // namespace Arc
