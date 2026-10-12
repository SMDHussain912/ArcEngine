// P1 Step 2: PhysicsWorld (Box2D 3.x) headless checks — no GL/context needed.
// Verifies: gravity integration, static bodies ignore gravity, fixed-rotation
// lock, velocity/impulse setters, teleport, and that a box comes to rest on a
// static floor (collision + gravity + solver working end to end).
// Run from the REPO ROOT: ./build/tests/test_physics
#include "ArcEngine/Physics/PhysicsWorld.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"
#include "ArcEngine/Scene/SceneSerializer.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 1;                                                         \
        }                                                                     \
    } while (0)

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-3f; }

// Advance the world by ~1 second in 1/60 slices (matches a 60fps loop).
static void RunSeconds(Arc::PhysicsWorld& pw, Arc::Scene& s, float seconds) {
    const float dt = 1.0f / 60.0f;
    int steps = static_cast<int>(seconds / dt);
    for (int i = 0; i < steps; ++i) pw.Step(s, dt);
}

int main() {
    // --- 1. Gravity integrates: a free body accelerates downward ---
    // Note: a Box2D body with no shape has zero mass, so gravity cannot move
    // it — every dynamic body here carries a collider (realistic usage).
    {
        Arc::Scene s;
        auto e = s.CreateEntity("Box");
        auto& tc = e.GetComponent<Arc::TransformComponent>();
        tc.Position = {0.0f, 100.0f, 0.0f};
        auto& rb = e.AddComponent<Arc::RigidBody2DComponent>();
        rb.Type = Arc::RigidBodyType::Dynamic;
        e.AddComponent<Arc::BoxCollider2DComponent>();

        Arc::PhysicsWorld pw;
        CHECK(pw.Create(), "world created");
        CHECK(pw.GetGravity().y == -9.8f, "default gravity -9.8");
        CHECK(pw.AddBody(s, e), "dynamic body added");
        CHECK(pw.BodyCount() == 1, "one body");
        CHECK(pw.AddBody(s, e), "AddBody idempotent");
        CHECK(pw.BodyCount() == 1, "still one body");

        RunSeconds(pw, s, 1.0f);
        float y = tc.Position.y;
        CHECK(y < 99.0f, "body fell under gravity");
        CHECK(pw.GetLinearVelocity(e).y < 0.0f, "downward velocity");
        CHECK(pw.GetLinearVelocity(e).y < -8.0f, "gravity ~ 9.8 m/s^2");
        pw.Destroy();
    }

    // --- 2. Static bodies ignore gravity ---
    {
        Arc::Scene s;
        auto e = s.CreateEntity("Ground");
        auto& tc = e.GetComponent<Arc::TransformComponent>();
        tc.Position = {0.0f, 0.0f, 0.0f};
        auto& rb = e.AddComponent<Arc::RigidBody2DComponent>();
        rb.Type = Arc::RigidBodyType::Static;

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, e);
        RunSeconds(pw, s, 1.0f);
        CHECK(Near(tc.Position.y, 0.0f), "static body stays put");
        pw.Destroy();
    }

    // --- 3. FixedRotation locks spin; impulse/velocity setters work ---
    {
        Arc::Scene s;
        auto e = s.CreateEntity("Crate");
        auto& tc = e.GetComponent<Arc::TransformComponent>();
        tc.Position = {0.0f, 50.0f, 0.0f};
        auto& rb = e.AddComponent<Arc::RigidBody2DComponent>();
        rb.Type = Arc::RigidBodyType::Dynamic;
        rb.FixedRotation = true;
        e.AddComponent<Arc::BoxCollider2DComponent>();

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, e);

        pw.SetLinearVelocity(e, {5.0f, 0.0f});
        CHECK(Near(pw.GetLinearVelocity(e).x, 5.0f), "set linear velocity x");
        RunSeconds(pw, s, 0.5f);
        CHECK(tc.Position.x > 1.5f, "velocity moved body +x");
        CHECK(Near(tc.Rotation.z, 0.0f), "fixed rotation kept angle 0");

        pw.ApplyImpulse(e, {0.0f, 1000.0f});
        CHECK(pw.GetLinearVelocity(e).y > 0.0f, "upward impulse raised vy");

        // Teleport overrides position immediately. Zero velocity first so the
        // follow-up step doesn't drift the body off the target (TransformComponent
        // syncs during Step, exactly like Unity's Rigidbody2D.MovePosition).
        pw.SetLinearVelocity(e, {0.0f, 0.0f});
        pw.Teleport(e, {-10.0f, -10.0f}, 1.0f);
        pw.Step(s, 1.0f / 60.0f);
        CHECK(Near(tc.Position.x, -10.0f), "teleport repositions body x");
        CHECK(tc.Position.y > -10.01f && tc.Position.y < -9.99f,
              "teleport repositions body y (one gravity step)");
        CHECK(Near(tc.Rotation.z, 1.0f), "teleport sets angle");
        pw.Destroy();
    }

    // --- 4. Collision: box lands on a static floor and rests (no tunneling) ---
    {
        Arc::Scene s;
        // Floor 100x1 centered at y=0 => top surface at y=+0.5.
        auto floor = s.CreateEntity("Floor");
        floor.GetComponent<Arc::TransformComponent>().Position = {0.0f, 0.0f, 0.0f};
        auto& frb = floor.AddComponent<Arc::RigidBody2DComponent>();
        frb.Type = Arc::RigidBodyType::Static;
        auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
        fcol.Size = {100.0f, 1.0f};

        // 1x1 box dropped from y=5 => rests near y=1.0 (floor top + half-height).
        auto box = s.CreateEntity("Box");
        auto& btc = box.GetComponent<Arc::TransformComponent>();
        btc.Position = {0.0f, 5.0f, 0.0f};
        auto& brb = box.AddComponent<Arc::RigidBody2DComponent>();
        brb.Type = Arc::RigidBodyType::Dynamic;
        auto& bcol = box.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Size = {1.0f, 1.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, floor);
        pw.AddBody(s, box);
        RunSeconds(pw, s, 3.0f);

        CHECK(btc.Position.y > 0.9f && btc.Position.y < 1.2f,
              "box rests on floor surface (no fall-through)");
        CHECK(std::fabs(pw.GetLinearVelocity(box).y) < 0.5f,
              "box settles to near-zero velocity");
        pw.Destroy();
    }

    // --- 4b. Shape selection: circle + capsule rest at THEIR heights ---
    // Floor top is y=0.5 (Size 100x1 at origin). A circle r rests at 0.5+r;
    // a capsule (segment H, cap radius r) rests at 0.5 + H/2 + r. These only
    // hold if AddBody created the right b2 shape — a box built from Size
    // 1x1 would also rest near y=1.0, so the capsule case (1.25) is the
    // one that truly distinguishes capsule vs box.
    {
        Arc::Scene s;
        auto floor = s.CreateEntity("Floor");
        auto& frb = floor.AddComponent<Arc::RigidBody2DComponent>();
        frb.Type = Arc::RigidBodyType::Static;
        auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
        fcol.Size = {100.0f, 1.0f}; // Radius/Height stay 0 => box

        auto ball = s.CreateEntity("Ball");
        ball.GetComponent<Arc::TransformComponent>().Position = {0.0f, 5.0f, 0.0f};
        ball.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bcol = ball.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Radius = 0.5f; // Height 0 => circle

        auto pill = s.CreateEntity("Pill");
        pill.GetComponent<Arc::TransformComponent>().Position = {3.0f, 5.0f, 0.0f};
        auto& prb = pill.AddComponent<Arc::RigidBody2DComponent>();
        prb.Type = Arc::RigidBodyType::Dynamic;
        prb.FixedRotation = true;
        auto& pcol = pill.AddComponent<Arc::BoxCollider2DComponent>();
        pcol.Radius = 0.25f;
        pcol.Height = 1.0f; // => capsule: total half-height 0.75

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, floor);
        pw.AddBody(s, ball);
        pw.AddBody(s, pill);
        RunSeconds(pw, s, 3.0f);

        float yBall = ball.GetComponent<Arc::TransformComponent>().Position.y;
        float yPill = pill.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(yBall > 0.9f && yBall < 1.1f,
              "circle rests at floor_top + radius (0.5 + 0.5)");
        CHECK(yPill > 1.1f && yPill < 1.4f,
              "capsule rests at floor_top + H/2 + r (0.5 + 0.5 + 0.25)");
        pw.Destroy();
    }

    // --- 4c. Polygon shape: diamond rests on its vertex; degenerate hull
    // falls back to box. Diamond half-diagonal 0.7 => rest y = 0.5 + 0.7 =
    // 1.2; a box (Size 1x1) would rest at 1.0, so only the real b2Polygon
    // passes the > 1.1 lower bound. ---
    {
        Arc::Scene s;
        auto floor = s.CreateEntity("Floor");
        auto& frb = floor.AddComponent<Arc::RigidBody2DComponent>();
        frb.Type = Arc::RigidBodyType::Static;
        auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
        fcol.Size = {100.0f, 1.0f}; // box (no radius/height/vertices)

        auto rock = s.CreateEntity("Rock");
        rock.GetComponent<Arc::TransformComponent>().Position = {0.0f, 5.0f, 0.0f};
        auto& rrb = rock.AddComponent<Arc::RigidBody2DComponent>();
        rrb.Type = Arc::RigidBodyType::Dynamic;
        rrb.FixedRotation = true; // stay vertex-down (no toppling)
        auto& rcol = rock.AddComponent<Arc::BoxCollider2DComponent>();
        rcol.VertexCount = 4;
        rcol.Vertices[0] = {0.7f, 0.0f};
        rcol.Vertices[1] = {0.0f, 0.7f};
        rcol.Vertices[2] = {-0.7f, 0.0f};
        rcol.Vertices[3] = {0.0f, -0.7f};

        // Degenerate polygon: three collinear points -> empty hull -> box
        // fallback (Size 1x1) with an error log. Must NOT crash and must
        // still land like a box.
        auto deg = s.CreateEntity("Deg");
        deg.GetComponent<Arc::TransformComponent>().Position = {3.0f, 5.0f, 0.0f};
        deg.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& dcol = deg.AddComponent<Arc::BoxCollider2DComponent>();
        dcol.VertexCount = 3;
        dcol.Vertices[0] = {0.0f, 0.0f};
        dcol.Vertices[1] = {0.5f, 0.0f};
        dcol.Vertices[2] = {1.0f, 0.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        CHECK(pw.AddBody(s, floor), "floor body added");
        CHECK(pw.AddBody(s, rock), "polygon body added");
        CHECK(pw.AddBody(s, deg), "degenerate polygon still adds (box fallback)");
        RunSeconds(pw, s, 3.0f);

        float yRock = rock.GetComponent<Arc::TransformComponent>().Position.y;
        float yDeg = deg.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(yRock > 1.1f && yRock < 1.3f,
              "diamond rests on vertex (0.5 + 0.7 = 1.2, box would be 1.0)");
        CHECK(yDeg > 0.9f && yDeg < 1.2f,
              "degenerate hull falls back to box rest height");
        pw.Destroy();
    }

    // --- 4d. Chain terrain: ONE-SIDED collision on the right of travel ---
    {
        // A: flat chain wound right->left (travel -x => right side +y = up).
        // A box dropped from above must rest ON the line (y = 0 + 0.5); if
        // the winding doc were wrong it falls through.
        Arc::Scene s;
        auto ground = s.CreateEntity("ChainGround");
        ground.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;
        auto& gcol = ground.AddComponent<Arc::BoxCollider2DComponent>();
        gcol.Chain = {{10.0f, 0.0f}, {-10.0f, 0.0f}};

        auto box = s.CreateEntity("Box");
        box.GetComponent<Arc::TransformComponent>().Position = {0.0f, 3.0f, 0.0f};
        box.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bcol = box.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Size = {1.0f, 1.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        CHECK(pw.AddBody(s, ground), "chain body added");
        CHECK(pw.AddBody(s, box), "dropper added");
        RunSeconds(pw, s, 3.0f);
        float yA = box.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(yA > 0.4f && yA < 0.6f,
              "box lands on walkable chain wound right-to-left");
        pw.Destroy();
    }
    {
        // B: one-sided proof. Same chain; box fired UP from y=-1. Unimpeded
        // it crosses y=0 and is at ~+2.04 after 0.6s. A two-sided chain
        // blocks it at the underside (~y=-0.5) and it falls back below.
        Arc::Scene s;
        auto ground = s.CreateEntity("ChainGround");
        ground.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;
        auto& gcol = ground.AddComponent<Arc::BoxCollider2DComponent>();
        gcol.Chain = {{10.0f, 0.0f}, {-10.0f, 0.0f}};

        auto box = s.CreateEntity("Box");
        box.GetComponent<Arc::TransformComponent>().Position = {0.0f, -1.0f, 0.0f};
        box.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bcol = box.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Size = {1.0f, 1.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, ground);
        pw.AddBody(s, box);
        pw.SetLinearVelocity(box, {0.0f, 8.0f});
        for (int i = 0; i < 36; ++i) pw.Step(s, 1.0f / 60.0f); // 0.6 s
        float yB = box.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(yB > 0.5f,
              "one-sided: box passed through the underside of the chain");
        pw.Destroy();
    }
    {
        // C: closed loop bowl, wound clockwise => interior on the right of
        // travel. The box passes through the top edge (collides only from
        // below) and rests on the bowl FLOOR at y = -1 + 0.5 = -0.5. A
        // two-sided loop would stop it on TOP at y = 1.5.
        Arc::Scene s;
        auto bowl = s.CreateEntity("Bowl");
        bowl.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;
        auto& bcol = bowl.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Chain = {{-1.0f, -1.0f}, {-1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, -1.0f}};
        bcol.ChainLoop = true;

        auto box = s.CreateEntity("Box");
        box.GetComponent<Arc::TransformComponent>().Position = {0.0f, 3.0f, 0.0f};
        box.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bbc = box.AddComponent<Arc::BoxCollider2DComponent>();
        bbc.Size = {1.0f, 1.0f};

        Arc::PhysicsWorld pw;
        pw.Create();
        CHECK(pw.AddBody(s, bowl), "loop chain added");
        CHECK(pw.AddBody(s, box), "bowl dropper added");
        RunSeconds(pw, s, 3.0f);
        float yC = box.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(yC > -0.7f && yC < -0.3f,
              "box inside loop bowl (entered through top, rests on floor)");
        pw.Destroy();
    }

    // --- 4e. Compound bodies: sensor feet + AddShape/RemoveShape API ---
    {
        // Sensor-feet pattern: solid primary box rests at floor_top + 0.5.
        // Two sensor circles hang BELOW the box bottom — they must report
        // sensor touches WITHOUT moving the body (no physical response).
        Arc::Scene s;
        auto floor = s.CreateEntity("Floor");
        floor.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;
        auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
        fcol.Size = {100.0f, 1.0f}; // top at y = 0.5

        auto player = s.CreateEntity("Player");
        player.GetComponent<Arc::TransformComponent>().Position = {0.0f, 3.0f, 0.0f};
        player.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& pcol = player.AddComponent<Arc::BoxCollider2DComponent>();
        pcol.Size = {1.0f, 1.0f}; // primary solid shape
        Arc::SubShape2D footL;
        footL.Shape = Arc::SubShape2D::Type::Circle;
        footL.Offset = {-0.3f, -0.7f};
        footL.Radius = 0.2f;
        footL.IsSensor = true;
        Arc::SubShape2D footR = footL;
        footR.Offset = {0.3f, -0.7f};
        pcol.SubShapes = {footL, footR};

        Arc::PhysicsWorld pw;
        pw.Create();
        CHECK(pw.AddBody(s, floor), "floor added");
        CHECK(pw.AddBody(s, player), "compound player added");
        RunSeconds(pw, s, 3.0f);

        float y = player.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(y > 0.9f && y < 1.1f,
              "sensor feet do not push the body (rests on primary box)");
        CHECK(pw.SensorTouched(s, player, floor),
              "sensor feet report overlap with the floor");
        // The primary still emits a SOLID contact alongside the sensors.
        auto evs = pw.PollCollisionEvents(s);
        bool solidContact = false;
        for (const auto& ev : evs) {
            if (!ev.begun || ev.sensor) continue;
            uint32_t a = ev.entityA.Id(), b = ev.entityB.Id();
            if ((a == player.Id() && b == floor.Id()) ||
                (a == floor.Id() && b == player.Id()))
                solidContact = true;
        }
        CHECK(solidContact, "primary box still emits solid contact events");
        pw.Destroy();
    }
    {
        // AddShape/RemoveShape rest-height transitions: primary 0.5 box rests
        // at 0.5 + 0.25 = 0.75; a live circle extending 0.9 below the body
        // lifts rest to 0.5 + 0.9 = 1.4; removing it drops back to 0.75.
        Arc::Scene s;
        auto floor = s.CreateEntity("Floor");
        floor.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Static;
        auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
        fcol.Size = {100.0f, 1.0f};

        auto box = s.CreateEntity("Box");
        box.GetComponent<Arc::TransformComponent>().Position = {0.0f, 3.0f, 0.0f};
        box.AddComponent<Arc::RigidBody2DComponent>().Type =
            Arc::RigidBodyType::Dynamic;
        auto& bcol = box.AddComponent<Arc::BoxCollider2DComponent>();
        bcol.Size = {0.5f, 0.5f};

        Arc::PhysicsWorld pw;
        pw.Create();
        pw.AddBody(s, floor);
        pw.AddBody(s, box);
        RunSeconds(pw, s, 2.0f);
        float y0 = box.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(y0 > 0.65f && y0 < 0.85f, "primary-only rest height (0.75)");

        Arc::SubShape2D ball;
        ball.Shape = Arc::SubShape2D::Type::Circle;
        ball.Offset = {0.0f, -0.4f};
        ball.Radius = 0.5f; // lowest point = -0.9 from body center
        CHECK(pw.AddShape(s, box, ball) == 0, "AddShape returns index 0");
        RunSeconds(pw, s, 2.0f);
        float y1 = box.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(y1 > 1.25f && y1 < 1.55f,
              "AddShape lifts body onto new circle (0.5 + 0.9 = 1.4)");

        CHECK(pw.RemoveShape(s, box, 0), "RemoveShape(0) succeeds");
        CHECK(!pw.RemoveShape(s, box, 0), "RemoveShape on empty fails");
        RunSeconds(pw, s, 2.0f);
        float y2 = box.GetComponent<Arc::TransformComponent>().Position.y;
        CHECK(y2 > 0.65f && y2 < 0.85f,
              "RemoveShape drops body back to primary rest height");
        pw.Destroy();
    }

    // --- 5. Entities without rigid bodies / post-destroy are safe no-ops ---
    {
        Arc::Scene s;
        auto e = s.CreateEntity("Plain");
        Arc::PhysicsWorld pw;
        pw.Create();
        CHECK(!pw.AddBody(s, e), "no rigid body => AddBody false");
        CHECK(!pw.HasBody(e), "no body registered");
        CHECK(!pw.RemoveBody(e), "removing absent body false");
        pw.SetLinearVelocity(e, {1.0f, 1.0f}); // must not crash
        CHECK(Near(pw.GetLinearVelocity(e).x, 0.0f), "absent body => zero velocity");
        RunSeconds(pw, s, 0.5f);               // must not crash
        pw.Destroy();
        CHECK(!pw.AddBody(s, e), "post-destroy AddBody false");
        CHECK(Near(pw.GetGravity().y, 0.0f), "post-destroy gravity zero");
    }

    // --- 6. Serialization round-trip: RigidBody2D + BoxCollider2D persist ---
    {
        Arc::Scene s;
        auto box = s.CreateEntity("Box");
        box.GetComponent<Arc::TransformComponent>().Position = {2.0f, 3.0f, 0.0f};
        auto& rb = box.AddComponent<Arc::RigidBody2DComponent>();
        rb.Type = Arc::RigidBodyType::Kinematic;
        rb.FixedRotation = true;
        rb.GravityScale = 0.25f;
        auto& col = box.AddComponent<Arc::BoxCollider2DComponent>();
        col.Size = {1.5f, 2.5f};
        col.Density = 2.0f;
        col.Friction = 0.6f;
        col.Restitution = 0.4f;
        col.IsSensor = true;

        const std::string dir = "/tmp/arc_physics_test";
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        const std::string path = dir + "/scene.arc";
        CHECK(Arc::SceneSerializer::Save(s, path), "save physics scene");

        Arc::Scene loaded;
        CHECK(Arc::SceneSerializer::Load(path, loaded), "load physics scene");

        auto entities = loaded.Entities();
        CHECK(entities.size() == 1, "one entity round-tripped");
        if (entities.size() == 1) {
            auto& e = entities[0];
            CHECK(e.HasComponent<Arc::RigidBody2DComponent>(), "rigid body persisted");
            CHECK(e.HasComponent<Arc::BoxCollider2DComponent>(), "collider persisted");
            const auto& lrb = e.GetComponent<Arc::RigidBody2DComponent>();
            CHECK(lrb.Type == Arc::RigidBodyType::Kinematic, "body type persisted");
            CHECK(lrb.FixedRotation, "fixed rotation persisted");
            CHECK(Near(lrb.GravityScale, 0.25f), "gravity scale persisted");
            const auto& lcol = e.GetComponent<Arc::BoxCollider2DComponent>();
            CHECK(Near(lcol.Size.x, 1.5f) && Near(lcol.Size.y, 2.5f), "collider size persisted");
            CHECK(Near(lcol.Density, 2.0f), "density persisted");
            CHECK(Near(lcol.Friction, 0.6f), "friction persisted");
            CHECK(Near(lcol.Restitution, 0.4f), "restitution persisted");
            CHECK(lcol.IsSensor, "sensor flag persisted");
        }
    }

    std::printf("test_physics: ALL OK\n");
    return 0;
}
