#pragma once
#include "ArcEngine/Core/Math.h"

#include <cstdint>
#include <string>
#include <vector>

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

// Extra shape on a compound body (P1 Step 9): positioned in body-local
// space, with its OWN Density/Friction/Restitution and IsSensor. Used via
// BoxCollider2DComponent::SubShapes (serialized) or PhysicsWorld::AddShape
// (runtime). Box/Circle/Capsule only — polygon/chain sub-shapes are out of
// scope for Step 9.
struct SubShape2D {
    enum class Type { Box = 0, Circle = 1, Capsule = 2 };
    Type Shape = Type::Box;
    Vec2 Offset{0.0f, 0.0f}; // body-local center
    Vec2 Size{0.5f, 0.5f};   // Box: full extents (SpriteComponent convention)
    float Radius = 0.25f;    // Circle radius / Capsule end-cap radius
    float Height = 0.0f;     // Capsule segment length
    float Density = 1.0f;
    float Friction = 0.3f;
    float Restitution = 0.0f;
    bool IsSensor = false;
};

// BoxCollider2D (P1 Step 2): box collision shape for the parent body. Size is
// the FULL width/height in world units (like SpriteComponent.Size); the physics
// wrapper converts it to Box2D half-extents. No collider = body with no shape.
struct BoxCollider2DComponent {
    Vec2 Size{1.0f, 1.0f};
    // Shape selection (P1 Steps 5-7), most-specific first:
    //   VertexCount >= 3 -> convex polygon (local-space Vertices, hull-checked)
    //   Height > 0       -> capsule (segment length Height, end-cap radius Radius)
    //   Radius  > 0      -> circle
    //   otherwise        -> box (half-extents from Size)
    // Defaults keep legacy scenes as boxes.
    float Radius = 0.0f;       // circle/capsule radius (0 = box)
    float Height = 0.0f;       // capsule segment length (0 = box/circle)
    // Polygon vertices in LOCAL space (P1 Step 7), fixed 8 = B2_MAX_POLYGON_VERTICES.
    // Invalid hulls (collinear/non-convex-to-be) fall back to box at AddBody with an error log.
    Vec2 Vertices[8]{};
    int VertexCount = 0;       // 0..8; >= 3 selects the polygon shape
    // Chain terrain outline (P1 Step 8): >= 2 points selects the chain shape
    // (one-sided — collides on the RIGHT side of the point order, so walkable
    // ground winds right-to-left or sets ChainFlip). ChainLoop closes last->
    // first (needs >= 3). EdgeCollider2D lite = a 2-point open chain.
    std::vector<Vec2> Chain;
    bool ChainLoop = false;
    bool ChainFlip = false;
    float Density = 1.0f;      // kg/m^2 (drives mass; ignored for static bodies)
    float Friction = 0.3f;     // Coulomb friction [0,1]
    float Restitution = 0.0f;  // bounciness [0,1]
    bool IsSensor = false;     // overlaps reported, no physical response
    // Compound extras (P1 Step 9): the fields above are the PRIMARY shape;
    // each sub-shape has its own material + sensor flag (Box2D sums all
    // shape densities for the combined mass).
    std::vector<SubShape2D> SubShapes;
};

// Joint2D (P1 Step 10): connects this entity's body (A) to another entity's
// body (B). Anchors are LOCAL-space offsets on each body; for Weld the two
// world anchors should coincide at rest. This Box2D build has NO world-attach
// joints, so the target must be a real entity (typically a static anchor).
// Created AFTER all bodies exist: PhysicsWorld::CreateJoint (editor Play runs
// it from RebuildPhysicsBodies).
struct Joint2DComponent {
    enum class Kind { Revolute = 0, Distance = 1, Prismatic = 2, Weld = 3 };
    Kind Type = Kind::Revolute;
    std::string TargetName;  // target entity name (stable across save/load)
    uint32_t TargetId = 0;   // resolved at CreateJoint (0 = unresolved)
    Vec2 AnchorA{0.0f, 0.0f}; // local on THIS body
    Vec2 AnchorB{0.0f, 0.0f}; // local on the TARGET body
    // Limits + motor (shared flags; Kind decides the units — Revolute:
    // radians / N*m torque; Prismatic: meters / N force).
    bool EnableLimit = false;
    float LowerAngle = 0.0f;
    float UpperAngle = 0.0f;                    // Revolute (radians)
    float LowerTranslation = -1.0f;
    float UpperTranslation = 1.0f;              // Prismatic (meters)
    bool EnableMotor = false;
    float MotorSpeed = 0.0f;                    // rad/s (Revolute) or m/s.
    // Box2D convention: speed = target(B) minus owner(A) angular velocity —
    // static target + dynamic owner spins the owner at -MotorSpeed.
    float MaxMotorForce = 0.0f;                 // N*m (Revolute) or N
    // Distance: rest length (< 0 = measure world anchor distance at creation).
    float Length = -1.0f;
    bool EnableLengthLimit = false;
    float MinLength = 0.0f;
    float MaxLength = 1.0f;
    // Box2D applies distance length-limits only in spring mode (the rigid
    // joint enforces `length` and overrides limits) — hence this switch.
    bool UseSpring = false;
    float Hertz = 1.0f;       // spring stiffness (Distance)
    float DampingRatio = 0.7f;
    // Prismatic: travel axis in WORLD space (mapped onto both local frames
    // at creation). Weld stiffness stays at Box2D defaults (Weld lite).
    Vec2 Axis{1.0f, 0.0f};
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

