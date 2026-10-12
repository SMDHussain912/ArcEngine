// P1 Step 4: collision events (contact begin/end + sensor begin/end), headless.
// Verifies: solid begin on landing; exactly-once drain; canonical pair order;
// sensor begin via SensorTouched (both arg orders); sensor end on teleport-out;
// multi-substep frames keep events; RemoveBody purges queued pairs; Destroy
// clears the buffer. Run from REPO ROOT: ./build/tests/test_physics_events
#include "ArcEngine/Physics/PhysicsWorld.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <utility>
#include <vector>

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

// Pair (a,b,begun,sensor) found in the drained list?
static bool HasEvent(const std::vector<Arc::PhysicsWorld::CollisionEvent>& evs,
                     uint32_t a, uint32_t b, bool begun, bool sensor) {
    if (b < a) std::swap(a, b); // canonical, mirrors the wrapper contract
    for (const auto& e : evs) {
        if (e.entityA.Id() == a && e.entityB.Id() == b &&
            e.begun == begun && e.sensor == sensor)
            return true;
    }
    return false;
}

// Static floor at y=0 (size 6x0.5, top at y=0.25) + dynamic box above it.
// PhysicsWorld is non-copyable (PIMPL), so the rig fills by reference.
struct DropRig {
    Arc::Scene s;
    Arc::PhysicsWorld pw;
    Arc::Entity floor;
    Arc::Entity box;
};

static void MakeDropRig(DropRig& r, float boxY, bool boxIsSensor) {
    r.pw.Create(); // default gravity (0, -9.8)
    r.floor = r.s.CreateEntity("Floor");
    auto& frb = r.floor.AddComponent<Arc::RigidBody2DComponent>();
    frb.Type = Arc::RigidBodyType::Static;
    auto& fcol = r.floor.AddComponent<Arc::BoxCollider2DComponent>();
    fcol.Size = {6.0f, 0.5f};

    r.box = r.s.CreateEntity("Box");
    r.box.GetComponent<Arc::TransformComponent>().Position = {0.0f, boxY, 0.0f};
    auto& brb = r.box.AddComponent<Arc::RigidBody2DComponent>();
    brb.Type = Arc::RigidBodyType::Dynamic;
    auto& bcol = r.box.AddComponent<Arc::BoxCollider2DComponent>();
    bcol.Size = {0.5f, 0.5f};
    bcol.IsSensor = boxIsSensor;

    r.pw.AddBody(r.s, r.floor);
    r.pw.AddBody(r.s, r.box);
}

int main() {
    // --- 1. Solid begin on landing + canonical order + exactly-once drain ---
    {
        DropRig r;
        MakeDropRig(r, 3.0f, false);
        for (int i = 0; i < 90; ++i) r.pw.Step(r.s, 1.0f / 60.0f); // fall, land
        auto evs = r.pw.PollCollisionEvents(r.s);
        CHECK(HasEvent(evs, r.floor.Id(), r.box.Id(), true, false),
              "solid begin event on landing");
        for (const auto& e : evs) // canonical: A <= B always
            CHECK(e.entityA.Id() <= e.entityB.Id(), "canonical pair order");
        auto evs2 = r.pw.PollCollisionEvents(r.s);
        CHECK(evs2.empty(), "drain is exactly-once (second poll empty)");
        r.pw.Destroy();
    }

    // --- 2. Sensor begin via SensorTouched (both arg orders), no solid event ---
    {
        DropRig r;
        MakeDropRig(r, 3.0f, true); // falling body IS the sensor
        CHECK(!r.pw.SensorTouched(r.s, r.box, r.floor), "no touch before overlap");
        for (int i = 0; i < 60; ++i) r.pw.Step(r.s, 1.0f / 60.0f); // fall through
        CHECK(r.pw.SensorTouched(r.s, r.box, r.floor), "sensor begin (sensor,other)");
        CHECK(r.pw.SensorTouched(r.s, r.floor, r.box), "sensor begin (other,sensor)");
        auto evs = r.pw.PollCollisionEvents(r.s);
        CHECK(HasEvent(evs, r.floor.Id(), r.box.Id(), true, true),
              "sensor begin in drained list");
        for (const auto& e : evs)
            CHECK(e.sensor, "sensor rig emits only sensor events");
        r.pw.Destroy();
    }

    // --- 3. Sensor end on teleport-out ---
    {
        DropRig r;
        MakeDropRig(r, 0.5f, true); // start overlapping the floor
        r.pw.Step(r.s, 1.0f / 60.0f);
        r.pw.PollCollisionEvents(r.s); // clear the begin
        r.pw.Teleport(r.box, {0.0f, 50.0f}); // fling far away
        for (int i = 0; i < 5; ++i) r.pw.Step(r.s, 1.0f / 60.0f);
        auto evs = r.pw.PollCollisionEvents(r.s);
        CHECK(HasEvent(evs, r.floor.Id(), r.box.Id(), false, true),
              "sensor end after teleport-out");
        r.pw.Destroy();
    }

    // --- 4. Multi-substep frame keeps events (dt = 3 fixed steps at once) ---
    {
        DropRig r;
        MakeDropRig(r, 0.3f, false); // falls 0.45m in one frame (=3 substeps of 1/60, zero g)
        r.pw.Step(r.s, 3.0f); // ONE Step call = 3 fixed substeps (zero g)
        auto evs = r.pw.PollCollisionEvents(r.s);
        CHECK(HasEvent(evs, r.floor.Id(), r.box.Id(), true, false),
              "begin survives multi-substep frame");
        r.pw.Destroy();
    }

    // --- 5. RemoveBody purges queued pairs (no stale begin survives) ---
    {
        DropRig r;
        MakeDropRig(r, 0.5f, false); // start touching: begin drains fast
        r.pw.Step(r.s, 1.0f / 60.0f);
        r.pw.RemoveBody(r.box); // body gone BEFORE any poll
        auto evs = r.pw.PollCollisionEvents(r.s);
        CHECK(!HasEvent(evs, r.floor.Id(), r.box.Id(), true, false),
              "removed body leaves no stale begin");
        r.pw.Destroy();
    }

    // --- 6. Destroy clears the buffer; peek-then-poll consistency ---
    {
        DropRig r;
        MakeDropRig(r, 0.5f, true);
        r.pw.Step(r.s, 1.0f / 60.0f);
        CHECK(r.pw.SensorTouched(r.s, r.box, r.floor), "peek sees begin pre-destroy");
        r.pw.Destroy();
        auto evs = r.pw.PollCollisionEvents(r.s);
        CHECK(evs.empty(), "destroy clears queued events");
        CHECK(!r.pw.SensorTouched(r.s, r.box, r.floor), "peek empty post-destroy");
    }

    std::printf("test_physics_events: ALL OK\n");
    return 0;
}
