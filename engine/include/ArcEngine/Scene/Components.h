#pragma once
#include "ArcEngine/Core/Math.h"

#include <cstdint>
#include <string>

namespace Arc {

// Position + rotation (euler radians) + scale (M5).
// GetMatrix() = Translate * Rotate(ZYX) * Scale — Unity Transform feel.
// M7 editor will edit these fields in Inspector.
struct TransformComponent {
    Vec3 Position{0.0f, 0.0f, 0.0f};
    Vec3 Rotation{0.0f, 0.0f, 0.0f}; // radians, XYZ euler
    Vec3 Scale{1.0f, 1.0f, 1.0f};

    Mat4 GetMatrix() const {
        Mat4 m(1.0f);
        m = glm::translate(m, Position);
        m = glm::rotate(m, Rotation.z, glm::vec3(0, 0, 1));
        m = glm::rotate(m, Rotation.y, glm::vec3(0, 1, 0));
        m = glm::rotate(m, Rotation.x, glm::vec3(1, 0, 0));
        m = glm::scale(m, Scale);
        return m;
    }
};

// 2D scene camera (Godot Camera2D / Unreal CameraActor-lite): which entity
// the game view follows, plus zoom. Editor viewport pans/zooms independently;
// at runtime the game reads the first entity carrying this component.
struct CameraComponent {
    bool Active = true;
    float Zoom = 1.0f; // world units visible vertically = 2 / Zoom
};

// 2D light (Godot PointLight2D / Unreal PointLight-lite): editor draws the
// radius ring; renderer hookup for real shading arrives with materials.
struct LightComponent {
    Vec3 Color{1.0f, 0.95f, 0.85f};
    float Intensity = 1.0f;
    float Radius = 3.0f;
};

// Sprite2D (Godot-style, P1): texture-backed quad with tint, flip, UV region
// (atlas/sub-sprite), pixel filter, and draw layer. The SpriteBatch renders
// all sprites in one indexed draw per distinct texture — 06-texture drew one
// VAO per sprite; that path stays for single quads.
struct SpriteComponent {
    std::string TexturePath;        // asset path, e.g. "assets/textures/test.png"
    Vec4 Tint{1.0f, 1.0f, 1.0f, 1.0f};
    Vec2 RegionMin{0.0f, 0.0f};     // UV rect inside the texture
    Vec2 RegionMax{1.0f, 1.0f};
    bool FlipX = false;
    bool FlipY = false;
    bool FilterLinear = true;
    int32_t Layer = 0;              // low draws first (Godot CanvasLayer-lite)
    Vec2 Size{1.0f, 1.0f};          // world-unit quad size before Transform scale
};

// GUI widget (Godot Control / Unreal WidgetComponent-lite): drawn as an
// overlay quad in the editor viewport; runtime input comes with UI focus work.
struct GuiComponent {
    enum class Kind { Button = 0, Label = 1, Panel = 2 };
    Kind Widget = Kind::Button;
    Vec2 Size{1.2f, 0.4f};
    Vec3 BgColor{0.16f, 0.22f, 0.34f};
    Vec3 TextColor{0.92f, 0.94f, 0.98f};
    char Text[64] = {"Button"};
};

} // namespace Arc

