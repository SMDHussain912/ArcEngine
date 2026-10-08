// Dear ImGui editor (M7). Platform/renderer backends are the ImGui upstream
// GLFW + OpenGL3 impls (vendored), reused unchanged. This header owns the
// editor lifecycle and wires the ArcEngine scene/renderer through it.
//
// Godot/Unreal-style layout: menu bar + toolbar, Hierarchy (entities),
// Inspector (selected entity), live Viewport (off-screen FBO sampled into
// ImGui), and a Console feed. Viewport has a 2D ortho camera (pan/zoom),
// click-select picking, and a translate/rotate/scale gizmo.
#pragma once
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/Renderer.h"
#include "ArcEngine/Renderer/Shader.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Arc {

class ArcEditor {
public:
    enum class GizmoMode { Translate = 0, Rotate = 1, Scale = 2 };
    enum class PlayState { Edit = 0, Playing = 1, Paused = 2 };

    bool Init(Window& window, Renderer* renderer, Scene& scene);
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Call once per frame around the panel draws (after game render).
    // dt is used for play-mode simulation + smooth viewport camera.
    void BeginFrame(float dt);
    void BeginFrame() { BeginFrame(1.0f / 60.0f); }
    void EndFrame();

    // Editor panels (call between BeginFrame/EndFrame while visible).
    void DrawAll(); // menu + toolbar + hierarchy + inspector + viewport + console
    void DrawMenuBar();
    void DrawToolbar();
    void HierarchyWindow();
    void InspectorWindow();
    void ViewportWindow();
    void ConsoleWindow();
    void StatusBar();

    void SetScene(Scene& scene);

    PlayState GetPlayState() const { return m_play; }
    bool IsPlaying() const { return m_play == PlayState::Playing; }
    bool SceneDirty() const { return m_dirty; }
    void MarkDirty() { m_dirty = true; }
    uint64_t FrameCount() const { return m_frames; }

private:
    struct UndoEntry {
        uint32_t entityId = 0;
        TransformComponent before;
        TransformComponent after;
    };

    void EnsureGL();
    void DestroyGL();
    void RenderSceneToFBO(int w, int h);
    void DrawGizmoOverlay(const ImVec2& imgMin, const ImVec2& imgMax);
    void UpdateGizmoDrag(const ImVec2& imgMin, const ImVec2& imgMax, int w, int h);
    void UpdateViewportInput(const ImVec2& imgMin, const ImVec2& imgMax, int w, int h);
    bool EntityAtScreen(const ImVec2& imgMin, const ImVec2& imgMax, int w, int h,
                        float mx, float my, Entity& out);
    ImVec2 WorldToScreen(const Vec3& world, const ImVec2& imgMin, const ImVec2& imgMax,
                         int w, int h) const;
    Vec3 ScreenToWorld(float mx, float my, const ImVec2& imgMin, const ImVec2& imgMax,
                       int w, int h) const;
    Mat4 ViewProj(int w, int h) const;
    void PushUndo(uint32_t id, const TransformComponent& before,
                  const TransformComponent& after);
    void Undo();
    void Redo();
    void AddLog(const std::string& msg);
    Entity DuplicateEntity(Entity src);
    void ResetCamera();
    void ApplyTheme();
    void Play();
    void Pause();
    void Stop();
    bool SaveScene(const std::string& path);
    bool LoadScene(const std::string& path);

    Scene* m_scene = nullptr;
    Window* m_window = nullptr;
    Renderer* m_renderer = nullptr;
    bool m_initialized = false;

    // Off-screen viewport resources (owned here, freed in Shutdown).
    uint32_t m_fbo = 0;
    uint32_t m_fboTex = 0;
    int m_fboW = 0;
    int m_fboH = 0;
    std::unique_ptr<Shader> m_sceneShader;
    std::unique_ptr<Shader> m_flatShader;
    uint32_t m_gridVao = 0;
    uint32_t m_gridVbo = 0;
    int m_gridVerts = 0;
    bool m_glReady = false;

    // 2D ortho camera + interaction state.
    Vec2 m_camCenter{0.0f, 0.0f};
    float m_camZoom = 1.0f; // visible world height = 2 / zoom
    bool m_panning = false;
    Vec2 m_panLast{0.0f, 0.0f};
    GizmoMode m_gizmo = GizmoMode::Translate;
    bool m_showGrid = true;
    float m_snap = 0.0f;
    int m_dragAxis = 0; // 0 none, 1 X, 2 Y (screen-space gizmo drag)
    bool m_dragging = false;
    TransformComponent m_dragBefore;
    Vec3 m_dragWorldStart{0.0f, 0.0f, 0.0f};
    bool m_inspectEditing = false;
    uint32_t m_inspectId = 0;
    TransformComponent m_inspectBefore;

    // Panels / undo / console state.
    char m_search[128] = {};
    char m_savePath[256] = {"assets/scenes/editor_scene.arc"};
    char m_loadPath[256] = {"assets/scenes/editor_scene.arc"};
    char m_rename[128] = {};
    uint32_t m_renamingId = 0;
    std::vector<UndoEntry> m_undo;
    std::vector<UndoEntry> m_redo;
    std::vector<std::string> m_console;
    // Pro-suite state: play mode, selection, stats.
    PlayState m_play = PlayState::Edit;
    Scene m_playBackup;
    bool m_hasPlayBackup = false;
    bool m_dirty = false;
    uint64_t m_frames = 0;
    float m_statusMsgTime = 0.0f;
    char m_statusMsg[256] = {};
    float m_viewFps = 60.0f;
    // Fixed layout anchor (computed in DrawAll from the main viewport).
    ImVec2 m_layoutOrigin{0, 0};
    ImVec2 m_layoutSize{1280, 720};
};

} // namespace Arc
