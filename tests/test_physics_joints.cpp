// P1 Step 10: joint behavior. Each joint kind is proven by an observable
// constraint: revolute pendulum keeps its pivot distance, distance keeps rod
// length (auto-measured), weld cantilevers against gravity, prismatic locks
// the y-axis, and the motor spins a free rotor to speed.
#include "ArcEngine/Physics/PhysicsWorld.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include <cmath>
#include <cstdio>

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

static void RunSeconds(Arc::PhysicsWorld& pw, Arc::Scene& s, float seconds) {
    int n = static_cast<int>(seconds * 60.0f);
    for (int i = 0; i < n; ++i) pw.Step(s, 1.0f / 60.0f);
}

static float Dist(float ax, float ay, float bx, float by) {
    float dx = bx - ax, dy = by - ay;
    return std::sqrt(dx * dx + dy * dy);
}

int main() {
    // --- 1. Revolute pendulum: pivot constraint holds through the swing ---
    // Pivot (0,5): owner = bob at (2,5), target = anchor at (0,5). Both
    // world frames coincide at the pivot: AnchorA=(-2,0) on the bob,
    // AnchorB=(0,0) on the anchor.
    {
        Arc::Scene s;
        Arc::Entity anchor = s.CreateEntity("Anchor");
        anchor.GetComponent<Arc::TransformComponent>().Position = {0.0f, 5.0f, 0.0f};
        anchor.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;

        Arc::Entity bob = s.CreateEntity("Bob");
        bob.GetComponent<Arc::TransformComponent>().Position = {2.0f, 5.0f, 0.0f};
        bob.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bcol = bob.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Radius = 0.3f; // circle
        auto& j = bob.AddComponent<Arc::Joint2DComponent>();
        j.Type = Arc::Joint2DComponent::Kind::Revolute;
        j.TargetName = "Anchor";
        // Frames must coincide in world space at rest: owner frame on the
        // bob at (2,5)+(-2,0)=(0,5), target frame on the anchor at (0,5).
        j.AnchorA = {-2.0f, 0.0f};
        j.AnchorB = {0.0f, 0.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        CHECK(pw.AddBody(s, anchor), "anchor body added");
        CHECK(pw.AddBody(s, bob), "bob body added");
        CHECK(pw.CreateJoint(s, bob), "revolute created");
        CHECK(pw.CreateJoint(s, bob), "CreateJoint idempotent");
        RunSeconds(pw, s, 3.0f);

        const auto& tp = bob.GetComponent<Arc::TransformComponent>().Position;
        float d = Dist(0.0f, 5.0f, tp.x, tp.y);
        CHECK(d > 1.85f && d < 2.15f,
              "pendulum keeps pivot distance through the swing");
        CHECK(pw.RemoveJoint(bob), "RemoveJoint removes");
        CHECK(!pw.RemoveJoint(bob), "RemoveJoint again fails");
        CHECK(!pw.CreateJoint(s, anchor), "no joint component -> false");
        pw.Destroy();
    }

    // --- 2. Distance rod (auto length): hangs at rod length, not free-fall ---
    {
        Arc::Scene s;
        Arc::Entity anchor = s.CreateEntity("Anchor");
        anchor.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;

        Arc::Entity rock = s.CreateEntity("Rock");
        rock.GetComponent<Arc::TransformComponent>().Position = {3.0f, 0.0f, 0.0f};
        rock.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& rcol = rock.AddComponent<Arc::BoxCollider2DComponent>();
        rcol.Size = {0.6f, 0.6f};
        auto& j = rock.AddComponent<Arc::Joint2DComponent>();
        j.Type = Arc::Joint2DComponent::Kind::Distance;
        j.TargetName = "Anchor";
        j.Length = -1.0f; // auto: world anchor distance (3.0) at creation

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, anchor);
        pw.AddBody(s, rock);
        CHECK(pw.CreateJoint(s, rock), "distance created");
        // Short-time freeze check: released from rest, gravity must have
        // started the swing (a frozen body would sit at y = 0).
        for (int i = 0; i < 15; ++i) pw.Step(s, 1.0f / 60.0f); // 0.25 s
        float yEarly =
            rock.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(yEarly < -0.1f, "swing started (not frozen)");
        RunSeconds(pw, s, 2.25f); // total 2.5 s

        const auto& tp = rock.GetComponent<Arc::TransformComponent>().Position;
        float d = Dist(0.0f, 0.0f, tp.x, tp.y);
        CHECK(d > 2.8f && d < 3.2f, "distance rod holds measured length");
        pw.Destroy();
    }

    // PART2: weld + prismatic + motor scenes go here (appended next).
    // --- 3. Weld cantilever: rigid lock holds against gravity ---
    // Owner = beam at (1,0); target = wall at origin. World frames coincide
    // at the beam center (1,0): AnchorA=(0,0), AnchorB=(1,0).
    {
        Arc::Scene s;
        Arc::Entity wall = s.CreateEntity("Wall");
        wall.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;

        Arc::Entity beam = s.CreateEntity("Beam");
        beam.GetComponent<Arc::TransformComponent>().Position = {1.0f, 0.0f, 0.0f};
        beam.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bcol = beam.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Size = {0.5f, 0.5f};
        auto& j = beam.AddComponent<Arc::Joint2DComponent>();
        j.Type = Arc::Joint2DComponent::Kind::Weld;
        j.TargetName = "Wall";
        // Coincident at (0,0): beam-local (0,0) meets wall-local (1,0).
        j.AnchorA = {0.0f, 0.0f};
        j.AnchorB = {1.0f, 0.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, wall);
        pw.AddBody(s, beam);
        CHECK(pw.CreateJoint(s, beam), "weld created");
        RunSeconds(pw, s, 2.0f);

        const auto& tp = beam.GetComponent<Arc::TransformComponent>().Position;
        CHECK(tp.x > 0.65f && tp.x < 1.35f && tp.y > -0.35f && tp.y < 0.35f,
              "weld cantilever holds the beam against gravity");
        pw.Destroy();
    }

    // --- 4. Prismatic: horizontal axis locks y against gravity ---
    {
        Arc::Scene s;
        Arc::Entity rail = s.CreateEntity("Rail");
        rail.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;

        Arc::Entity cart = s.CreateEntity("Cart");
        cart.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& ccol = cart.AddComponent<Arc::BoxCollider2DComponent>();
        ccol.Size = {0.4f, 0.4f};
        auto& j = cart.AddComponent<Arc::Joint2DComponent>();
        j.Type = Arc::Joint2DComponent::Kind::Prismatic;
        j.TargetName = "Rail";
        j.Axis = {1.0f, 0.0f};
        j.EnableLimit = true;
        j.LowerTranslation = -2.0f;
        j.UpperTranslation = 2.0f;

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, rail);
        pw.AddBody(s, cart);
        CHECK(pw.CreateJoint(s, cart), "prismatic created");
        RunSeconds(pw, s, 2.0f);

        const auto& tp = cart.GetComponent<Arc::TransformComponent>().Position;
        CHECK(std::fabs(tp.y) < 0.1f,
              "prismatic axis locks y (would free-fall without the joint)");
        pw.Destroy();
    }

    // --- 5. Revolute motor-lite: spins a free rotor to commanded speed ---
    // Pivot coincides with the rotor's own center: no gravity torque, and
    // motor torque spins it in place.
    {
        Arc::Scene s;
        Arc::Entity hub = s.CreateEntity("Hub");
        hub.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;

        Arc::Entity rotor = s.CreateEntity("Rotor");
        rotor.GetComponent<Arc::TransformComponent>().Position = {1.0f, 0.0f, 0.0f};
        rotor.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& rcol = rotor.AddComponent<Arc::BoxCollider2DComponent>();
        rcol.Radius = 0.3f;
        auto& j = rotor.AddComponent<Arc::Joint2DComponent>();
        j.Type = Arc::Joint2DComponent::Kind::Revolute;
        j.TargetName = "Hub";
        j.AnchorA = {0.0f, 0.0f};
        j.AnchorB = {1.0f, 0.0f};
        j.EnableMotor = true;
        j.MotorSpeed = 3.0f;
        j.MaxMotorForce = 50.0f;

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, hub);
        pw.AddBody(s, rotor);
        CHECK(pw.CreateJoint(s, rotor), "motor joint created");
        RunSeconds(pw, s, 1.5f);

        float w = pw.GetAngularVelocity(rotor);
        // Box2D revolute motor speed is target-minus-owner angular velocity:
        // static hub (B) + dynamic owner (A) => the owner spins at -speed.
        CHECK(std::fabs(w + 3.0f) < 1.0f,
              "motor spins rotor to commanded speed (no motor => ~0)");
        pw.Destroy();
    }

    std::printf("test_physics_joints: ALL OK\n");
    return 0;
}