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

// RigidBody2D (P1 Step 2): how an entity participates in the Box2D world.
// Data only — PhysicsWorld turns this into a b2Body (see Physics/PhysicsWorld.h).
// Static = immovable (walls/ground), Kinematic = moved by velocity/code but not
// by forces, Dynamic = fully simulated (falls, collides, bounces).
enum class RigidBodyType { Static = 0, Kinematic = 1, Dynamic = 2 };

struct RigidBody2DComponent {
    RigidBodyType Type = RigidBodyType::Dynamic;
    bool FixedRotation = false; // lock Z-axis spin (platformer/top-down lite)
    float GravityScale = 1.0f;  // 0 = float, >1 = heavy
    float LinearDamping = 0.0f; // air drag (Box2D: velocity decay per step)
    float AngularDamping = 0.01f;
    // Collision filtering (P1 Step 3). Two bodies collide when each one's Mask
    // accepts the other's Category: (A.Mask & B.Category) && (B.Mask & A.Category).
    // Category is the layer this body occupies (one bit, e.g. 0x1); Mask is the set
    // of layers it collides with. GroupIndex overrides both: same positive group =>
    // always collide, same negative group => never collide (e.g. ragdoll parts).
    uint32_t Category = 0x00000001u;
    uint32_t Mask = 0xFFFFFFFFu;
    int GroupIndex = 0;
};

// BoxCollider2D (P1 Step 2): box collision shape for the parent body. Size is
// the FULL width/height in world units (like SpriteComponent.Size); the physics
// wrapper converts it to Box2D half-extents. No collider = body with no shape.
struct BoxCollider2DComponent {
    Vec2 Size{1.0f, 1.0f};
    float Radius = 0.5f;       // radius for circle/capsule (ignored for box)
    float Density = 1.0f;      // kg/m^2 (drives mass; ignored for static bodies)
    float Friction = 0.3f;     // Coulomb friction [0,1]
    float Restitution = 0.0f;  // bounciness [0,1]
    bool IsSensor = false;     // overlaps reported, no physical response
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

