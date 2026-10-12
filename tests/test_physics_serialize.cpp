// P1 Step 2: physics component round-trip through the .arc serializer. The
// PhysicsWorld itself needs a GL context only for rendering (it does not), but
// serialization is pure data, so this verifies every RigidBody2D / BoxCollider2D
// field survives save -> load, plus that entities without physics stay clean.
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"
#include "ArcEngine/Scene/SceneSerializer.h"

#include <cmath>
#include <cstdio>
#include <string>

static bool Near(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    Arc::Scene sceneA;

    // A fully-populated physics entity: dynamic body + bouncy box collider.
    Arc::Entity box = sceneA.CreateEntity("Box");
    box.GetComponent<Arc::TransformComponent>().Position = {3.0f, -2.0f, 0.0f};
    auto& rb = box.AddComponent<Arc::RigidBody2DComponent>();
    rb.Type = Arc::RigidBodyType::Dynamic;
    rb.FixedRotation = true;
    rb.GravityScale = 0.5f;
    rb.LinearDamping = 0.2f;
    rb.AngularDamping = 0.05f;
    rb.Category = 0x00000004u;
    rb.Mask = 0x0000000Fu;
    rb.GroupIndex = 3;
    auto& col = box.AddComponent<Arc::BoxCollider2DComponent>();
    col.Size = {2.5f, 1.25f};
    col.Density = 3.0f;
    col.Friction = 0.75f;
    col.Restitution = 0.4f;
    col.IsSensor = true;

    // A static ground body (kinematic default must NOT leak in).
    Arc::Entity ground = sceneA.CreateEntity("Ground");
    auto& grb = ground.AddComponent<Arc::RigidBody2DComponent>();
    grb.Type = Arc::RigidBodyType::Static;
    auto& gcol = ground.AddComponent<Arc::BoxCollider2DComponent>();
    gcol.Size = {8.0f, 0.5f};

    // Circle collider (P1 Step 5): Radius > 0 with Height == 0.
    Arc::Entity ball = sceneA.CreateEntity("Ball");
    ball.GetComponent<Arc::TransformComponent>().Position = {0.0f, -5.0f, 0.0f};
    ball.AddComponent<Arc::RigidBody2DComponent>().Type = Arc::RigidBodyType::Dynamic;
    auto& ballCol = ball.AddComponent<Arc::BoxCollider2DComponent>();
    ballCol.Radius = 0.6f;
    ballCol.IsSensor = true;

    // Capsule collider (P1 Step 6): Height > 0 (segment length) + Radius caps.
    Arc::Entity pill = sceneA.CreateEntity("Pill");
    pill.GetComponent<Arc::TransformComponent>().Position = {2.0f, -5.0f, 0.0f};
    pill.AddComponent<Arc::RigidBody2DComponent>().Type = Arc::RigidBodyType::Dynamic;
    auto& pillCol = pill.AddComponent<Arc::BoxCollider2DComponent>();
    pillCol.Radius = 0.25f;
    pillCol.Height = 1.0f;

    // Polygon collider (P1 Step 7): >= 3 local-space convex vertices (diamond).
    Arc::Entity rock = sceneA.CreateEntity("Rock");
    rock.GetComponent<Arc::TransformComponent>().Position = {4.0f, -5.0f, 0.0f};
    rock.AddComponent<Arc::RigidBody2DComponent>().Type = Arc::RigidBodyType::Dynamic;
    auto& rockCol = rock.AddComponent<Arc::BoxCollider2DComponent>();
    rockCol.VertexCount = 4;
    rockCol.Vertices[0] = {0.7f, 0.0f};
    rockCol.Vertices[1] = {0.0f, 0.7f};
    rockCol.Vertices[2] = {-0.7f, 0.0f};
    rockCol.Vertices[3] = {0.0f, -0.7f};

    // Chain terrain (P1 Step 8): >= 2 points, one-sided, with loop+flip flags.
    Arc::Entity terrain = sceneA.CreateEntity("Terrain");
    terrain.GetComponent<Arc::TransformComponent>().Position = {6.0f, -5.0f, 0.0f};
    terrain.AddComponent<Arc::RigidBody2DComponent>().Type =
        Arc::RigidBodyType::Static;
    auto& terrCol = terrain.AddComponent<Arc::BoxCollider2DComponent>();
    terrCol.Chain = {{2.0f, 0.0f}, {0.0f, 1.0f}, {-2.0f, 0.0f}};
    terrCol.ChainLoop = true;
    terrCol.ChainFlip = true;

    // Compound body (P1 Step 9): primary box + sensor foot + material bump.
    Arc::Entity walker = sceneA.CreateEntity("Walker");
    walker.GetComponent<Arc::TransformComponent>().Position = {8.0f, -5.0f, 0.0f};
    walker.AddComponent<Arc::RigidBody2DComponent>().Type =
        Arc::RigidBodyType::Dynamic;
    auto& walkCol = walker.AddComponent<Arc::BoxCollider2DComponent>();
    walkCol.Size = {0.6f, 1.0f};
    Arc::SubShape2D foot;
    foot.Shape = Arc::SubShape2D::Type::Circle;
    foot.Offset = {0.2f, -0.6f};
    foot.Radius = 0.15f;
    foot.IsSensor = true;
    walkCol.SubShapes.push_back(foot);
    Arc::SubShape2D bump;
    bump.Shape = Arc::SubShape2D::Type::Box;
    bump.Offset = {-0.4f, 0.3f};
    bump.Size = {0.2f, 0.2f};
    bump.Density = 3.0f;
    bump.Restitution = 0.9f;
    walkCol.SubShapes.push_back(bump);
    // Revolute joint pinning the walker to the ground (P1 Step 10).
    auto& wj = walker.AddComponent<Arc::Joint2DComponent>();
    wj.Type = Arc::Joint2DComponent::Kind::Revolute;
    wj.TargetName = "Ground";
    wj.AnchorA = {0.0f, 0.5f};
    wj.AnchorB = {8.0f, -4.5f};
    wj.EnableMotor = true;
    wj.MotorSpeed = 2.0f;
    wj.MaxMotorForce = 10.0f;

    // A bare entity: must stay free of physics components after round-trip.
    Arc::Entity plain = sceneA.CreateEntity("Plain");

    const char* tmpPath = "/tmp/arc_physics_roundtrip.arc";
    CHECK(Arc::SceneSerializer::Save(sceneA, tmpPath), "physics scene saves");

    Arc::Scene loaded;
    CHECK(Arc::SceneSerializer::Load(tmpPath, loaded), "physics scene loads");

    bool foundBox = false, foundGround = false, foundPlain = false;
    for (Arc::Entity e : loaded.Entities()) {
        if (e.Name() == "Plain") {
            foundPlain = true;
            CHECK(!e.HasComponent<Arc::RigidBody2DComponent>(),
                  "plain entity gains no rigidbody");
            CHECK(!e.HasComponent<Arc::BoxCollider2DComponent>(),
                  "plain entity gains no collider");
            continue;
        }
        if (e.Name() != "Box" && e.Name() != "Ground") continue;
        CHECK(e.HasComponent<Arc::RigidBody2DComponent>(),
              (e.Name() + " has rigidbody").c_str());
        CHECK(e.HasComponent<Arc::BoxCollider2DComponent>(),
              (e.Name() + " has collider").c_str());
        const auto& lrb = e.GetComponent<Arc::RigidBody2DComponent>();
        const auto& lcol = e.GetComponent<Arc::BoxCollider2DComponent>();

        if (e.Name() == "Box") {
            foundBox = true;
            CHECK(lrb.Type == Arc::RigidBodyType::Dynamic, "body type round-trip");
            CHECK(lrb.FixedRotation, "fixed_rotation round-trip");
            CHECK(Near(lrb.GravityScale, 0.5f), "gravity_scale round-trip");
            CHECK(Near(lrb.LinearDamping, 0.2f), "linear_damping round-trip");
            CHECK(Near(lrb.AngularDamping, 0.05f), "angular_damping round-trip");
            CHECK(lrb.Category == 0x00000004u, "category round-trip");
            CHECK(lrb.Mask == 0x0000000Fu, "mask round-trip");
            CHECK(lrb.GroupIndex == 3, "group_index round-trip");
            CHECK(Near(lcol.Size.x, 2.5f) && Near(lcol.Size.y, 1.25f),
                  "collider size round-trip");
            CHECK(Near(lcol.Density, 3.0f), "density round-trip");
            CHECK(Near(lcol.Friction, 0.75f), "friction round-trip");
            CHECK(Near(lcol.Restitution, 0.4f), "restitution round-trip");
            CHECK(Near(lcol.Radius, 0.0f), "radius default round-trip (0 = box)");
            CHECK(Near(lcol.Height, 0.0f), "height default round-trip (0 = not capsule)");
            CHECK(lcol.IsSensor, "is_sensor round-trip");
            // Transform coexists with physics and is preserved.
            CHECK(Near(e.GetComponent<Arc::TransformComponent>().Position.x, 3.0f),
                  "transform + physics coexist");
        } else { // Ground
            foundGround = true;
            CHECK(lrb.Type == Arc::RigidBodyType::Static, "static type round-trip");
            CHECK(!lrb.FixedRotation, "default fixed_rotation is false");
            CHECK(Near(lcol.Size.x, 8.0f) && Near(lcol.Size.y, 0.5f),
                  "ground size round-trip");
            CHECK(!lcol.IsSensor, "default is_sensor is false");
        }
    }
    CHECK(foundBox, "Box entity survives save/load");
    CHECK(foundGround, "Ground entity survives save/load");
    // Circle + capsule + polygon + chain + compound collider round-trips (5-9).
    bool foundBall = false, foundPill = false, foundRock = false,
         foundTerrain = false, foundWalker = false;
    for (Arc::Entity e : loaded.Entities()) {
        if (e.Name() == "Ball") {
            foundBall = true;
            const auto& c = e.GetComponent<Arc::BoxCollider2DComponent>();
            CHECK(Near(c.Radius, 0.6f), "circle radius round-trip");
            CHECK(Near(c.Height, 0.0f), "circle height stays 0");
            CHECK(c.IsSensor, "ball is_sensor round-trip");
        } else if (e.Name() == "Pill") {
            foundPill = true;
            const auto& c = e.GetComponent<Arc::BoxCollider2DComponent>();
            CHECK(Near(c.Radius, 0.25f), "capsule cap radius round-trip");
            CHECK(Near(c.Height, 1.0f), "capsule height round-trip");
        } else if (e.Name() == "Rock") {
            foundRock = true;
            const auto& c = e.GetComponent<Arc::BoxCollider2DComponent>();
            CHECK(c.VertexCount == 4, "polygon vertex count round-trip");
            CHECK(Near(c.Vertices[0].x, 0.7f) && Near(c.Vertices[0].y, 0.0f),
                  "polygon vertex 0 round-trip");
            CHECK(Near(c.Vertices[3].x, 0.0f) && Near(c.Vertices[3].y, -0.7f),
                  "polygon vertex 3 round-trip");
            CHECK(Near(c.Radius, 0.0f) && Near(c.Height, 0.0f),
                  "rock has no radius/height (polygon signal only)");
            CHECK(c.Chain.empty(), "rock has no chain");
        } else if (e.Name() == "Terrain") {
            foundTerrain = true;
            const auto& c = e.GetComponent<Arc::BoxCollider2DComponent>();
            CHECK(c.Chain.size() == 3, "chain point count round-trip");
            CHECK(c.ChainLoop, "chain_loop round-trip");
            CHECK(c.ChainFlip, "chain_flip round-trip");
            if (c.Chain.size() == 3) {
                CHECK(Near(c.Chain[0].x, 2.0f) && Near(c.Chain[0].y, 0.0f),
                      "chain p0 round-trip");
                CHECK(Near(c.Chain[2].x, -2.0f) && Near(c.Chain[2].y, 0.0f),
                      "chain p2 round-trip");
            }
            CHECK(c.VertexCount == 0, "terrain is chain-only (no polygon)");
        } else if (e.Name() == "Walker") {
            foundWalker = true;
            const auto& c = e.GetComponent<Arc::BoxCollider2DComponent>();
            CHECK(c.SubShapes.size() == 2, "subshape count round-trip");
            if (c.SubShapes.size() == 2) {
                const auto& f = c.SubShapes[0];
                CHECK(f.Shape == Arc::SubShape2D::Type::Circle,
                      "foot type round-trip");
                CHECK(Near(f.Offset.x, 0.2f) && Near(f.Offset.y, -0.6f),
                      "foot offset round-trip");
                CHECK(Near(f.Radius, 0.15f), "foot radius round-trip");
                CHECK(f.IsSensor, "foot sensor round-trip");
                const auto& b = c.SubShapes[1];
                CHECK(b.Shape == Arc::SubShape2D::Type::Box, "bump type round-trip");
                CHECK(Near(b.Offset.x, -0.4f), "bump offset round-trip");
                CHECK(Near(b.Size.x, 0.2f) && Near(b.Size.y, 0.2f),
                      "bump size round-trip");
                CHECK(Near(b.Density, 3.0f) && Near(b.Restitution, 0.9f),
                      "bump per-shape material round-trip");
                CHECK(!b.IsSensor, "bump is solid");
            }
            CHECK(e.HasComponent<Arc::Joint2DComponent>(), "joint persists");
            const auto& lj = e.GetComponent<Arc::Joint2DComponent>();
            CHECK(lj.Type == Arc::Joint2DComponent::Kind::Revolute,
                  "joint kind round-trip");
            CHECK(lj.TargetName == "Ground", "joint target name round-trip");
            CHECK(Near(lj.AnchorA.x, 0.0f) && Near(lj.AnchorA.y, 0.5f),
                  "joint anchorA round-trip");
            CHECK(Near(lj.AnchorB.x, 8.0f) && Near(lj.AnchorB.y, -4.5f),
                  "joint anchorB round-trip");
            CHECK(lj.EnableMotor, "joint motor flag round-trip");
            CHECK(Near(lj.MotorSpeed, 2.0f), "joint motor speed round-trip");
            CHECK(Near(lj.MaxMotorForce, 10.0f), "joint motor force round-trip");
        }
    }
    CHECK(foundBall, "Ball entity survives save/load");
    CHECK(foundPill, "Pill entity survives save/load");
    CHECK(foundRock, "Rock entity survives save/load");
    CHECK(foundTerrain, "Terrain entity survives save/load");
    CHECK(foundWalker, "Walker entity survives save/load");

    std::remove(tmpPath);
    std::printf("test_physics_serialize: ALL OK\n");
    return 0;
}


