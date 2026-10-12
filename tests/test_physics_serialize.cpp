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
            CHECK(Near(lcol.Radius, 0.5f), "radius default round-trip");
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
    // Circle collider round-trip    CHECK(foundBall, "Ball entity survives save/load");

    std::remove(tmpPath);
    std::printf("test_physics_serialize: ALL OK\n");
    return 0;
}


