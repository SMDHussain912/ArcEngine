// Dear ImGui editor (M7). Platform/renderer backends are the ImGui upstream
// GLFW + OpenGL3 impls (vendored), reused unchanged. This header owns the
// editor lifecycle and wires the ArcEngine scene/renderer through it.
#pragma once
#include "ArcEngine/Core/Window.h"
#include "ArcEngine/Renderer/Renderer.h"
#include "ArcEngine/Scene/Scene.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace Arc {

// M7 headless-free editor window: Hierarchy (entities) + Inspector
// (Transform/Sprite/Material/Audio stubs) + Viewport (live scene rendering).
class ArcEditor {
public:
    // Must be called BEFORE GraphicsContext::Init (implements the GLFW GL3
    // context *now* — needs the context bound).
    static bool Init(Window& window, Renderer* renderer, Scene& scene);
    static void Shutdown();

    // Call once per frame after Input::EndFrame (or before Render depending
    // on the window layout).
    static void BeginFrame();
    static void EndFrame();

    // Editor windows
    static void HierarchyWindow();
    static void InspectorWindow();
    static void ViewportWindow();

    static void SetScene(Scene& scene) { s_scene = &scene; }

private:
    static Scene* s_scene;
};

} // namespace Arc
