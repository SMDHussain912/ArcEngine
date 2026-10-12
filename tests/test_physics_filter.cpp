// P1 Step 3: collision filtering + raycast, headless (no GL/context).
//
// Verifies:
//  * Category/Mask gating: a dynamic box either rests on a static floor
//    (masks overlap) or falls straight through it (masks disjoint).
//  * GroupIndex override: negative group forces NO collision even when masks
//    match; positive group forces collision even when masks are disjoint.
//  * Raycast: returns the closest hit with a sane point/normal, maps back to the
//    hit ECS entity, reports miss on empty space, respects category/mask
//    filtering (a body whose mask excludes the query's category is not hit),
//    and prefers the nearer of two overlapping bodies.
// Run from the REPO ROOT: ./build/tests/test_physics_filter
#include "ArcEngine/Physics/PhysicsWorld.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include <cmath>
#include <cstdio>

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

static bool Near(float a, float b, float eps = 0.05f) {
    return std::fabs(a - b) < eps;
}

// Drop a dynamic box from y=3 onto a static floor at y=0 and report where the
// box settles after `seconds`. Both share size 0.5 (half 0.25): on a hit the
// box rests at y~=0.5; on a miss it plummets far below the floor.
static float DropAndSettleY(uint32_t boxMask, uint32_t floorCategory,
                            uint32_t floorMask, int boxGroup, int floorGroup) {
    Arc::Scene s;
    Arc::PhysicsWorld pw;
    pw.Create(); // default gravity (0, -9.8)

    Arc::Entity floor = s.CreateEntity("Floor");
    auto& frb = floor.AddComponent<Arc::RigidBody2DComponent>();
    frb.Type = Arc::RigidBodyType::Static;
    frb.Category = floorCategory;
    frb.Mask = floorMask;
    frb.GroupIndex = floorGroup;
    auto& fcol = floor.AddComponent<Arc::BoxCollider2DComponent>();
    fcol.Size = {6.0f, 0.5f};

    Arc::Entity box = s.CreateEntity("Box");
    box.GetComponent<Arc::TransformComponent>().Position = {0.0f, 3.0f, 0.0f};
    auto& brb = box.AddComponent<Arc::RigidBody2DComponent>();
    brb.Type = Arc::RigidBodyType::Dynamic;
    brb.Category = 0x00000001u; // box always lives on layer 1
    brb.Mask = boxMask;
    brb.GroupIndex = boxGroup;
    auto& bcol = box.AddComponent<Arc::BoxCollider2DComponent>();
    bcol.Size = {0.5f, 0.5f};

    pw.AddBody(s, floor);
    pw.AddBody(s, box);
    for (int i = 0; i < 180; ++i) pw.Step(s, 1.0f / 60.0f); // ~3 s
    return box.GetComponent<Arc::TransformComponent>().Position.y;
}

int main() {
    const uint32_t ALL = 0xFFFFFFFFu;

    // --- 1. Category/Mask: box mask accepts floor layer -> rests on floor ---
    // box(mask=0x1) vs floor(category=0x1): (0x1 & 0x1) && (0x1 & ALL) => collide.
    {
        float y = DropAndSettleY(/*boxMask*/ 0x00000001u, /*floorCat*/ 0x00000001u,
                                 /*floorMask*/ ALL, /*groups*/ 0, 0);
        CHECK(y > 0.3f && y < 0.7f, "masked-in box rests on floor");
    }

    // --- 2. Category/Mask: disjoint masks -> box falls through the floor ---
    // box(mask=0x2) vs floor(category=0x1): (0x2 & 0x1)==0 => no collision.
    {
        float y = DropAndSettleY(/*boxMask*/ 0x00000002u, /*floorCat*/ 0x00000001u,
                                 /*floorMask*/ ALL, /*groups*/ 0, 0);
        CHECK(y < -5.0f, "masked-out box falls through floor");
    }

    // --- 3. GroupIndex negative: never collide even though masks match ---
    {
        float y = DropAndSettleY(/*boxMask*/ ALL, /*floorCat*/ 0x00000001u,
                                 /*floorMask*/ ALL, /*boxGroup*/ -1, /*floorGroup*/ -1);
        CHECK(y < -5.0f, "negative groupIndex disables collision");
    }

    // --- 4. GroupIndex positive: always collide even with disjoint masks ---
    {
        float y = DropAndSettleY(/*boxMask*/ 0x00000002u, /*floorCat*/ 0x00000004u,
                                 /*floorMask*/ 0x00000008u, /*boxGroup*/ 7,
                                 /*floorGroup*/ 7);
        CHECK(y > 0.3f && y < 0.7f, "positive groupIndex forces collision");
    }

    // --- 5. Raycast: hit, closest body, sane point/normal, entity mapping ---
    {
        Arc::Scene s;
        Arc::PhysicsWorld pw;
        pw.Create({0.0f, 0.0f}); // no gravity: keep things where we put them

        // Far body at y = -5, near body at y = 0 (both 1x1 boxes).
        Arc::Entity far = s.CreateEntity("Far");
        far.GetComponent<Arc::TransformComponent>().Position = {0.0f, -5.0f, 0.0f};
        far.AddComponent<Arc::RigidBody2DComponent>().Type = Arc::RigidBodyType::Static;
        far.AddComponent<Arc::BoxCollider2DComponent>().Size = {1.0f, 1.0f};

        Arc::Entity near = s.CreateEntity("Near");
        near.GetComponent<Arc::TransformComponent>().Position = {0.0f, 0.0f, 0.0f};
        near.AddComponent<Arc::RigidBody2DComponent>().Type = Arc::RigidBodyType::Static;
        near.AddComponent<Arc::BoxCollider2DComponent>().Size = {1.0f, 1.0f};

        pw.AddBody(s, far);
        pw.AddBody(s, near);

        // Ray straight down the +x=0 line: must hit "Near" first.
        auto hit = pw.Raycast(s, {0.0f, 5.0f}, {0.0f, -10.0f});
        CHECK(hit.hit, "raycast hits a body");
        CHECK(hit.entity.Valid() && hit.entity.Id() == near.Id(),
              "raycast returns nearest body's entity");
        CHECK(Near(hit.point.y, 0.5f), "raycast impact on top face (y~=0.5)");
        CHECK(Near(hit.point.x, 0.0f), "raycast impact centered (x~=0)");
        CHECK(Near(hit.normal.y, 1.0f) && Near(hit.normal.x, 0.0f),
              "raycast normal points up off top face");
        CHECK(hit.fraction > 0.4f && hit.fraction < 0.5f,
              "raycast fraction ~0.45 down the 10-long ray");
    }

    // --- 6. Raycast miss: empty space returns hit==false ---
    {
        Arc::Scene s;
        Arc::PhysicsWorld pw;
        pw.Create({0.0f, 0.0f});
        Arc::Entity wall = s.CreateEntity("Wall");
        wall.GetComponent<Arc::TransformComponent>().Position = {0.0f, 0.0f, 0.0f};
        wall.AddComponent<Arc::RigidBody2DComponent>().Type = Arc::RigidBodyType::Static;
        wall.AddComponent<Arc::BoxCollider2DComponent>().Size = {1.0f, 1.0f};
        pw.AddBody(s, wall);

        auto miss = pw.Raycast(s, {10.0f, 10.0f}, {0.0f, -5.0f}); // off to the side
        CHECK(!miss.hit, "raycast reports miss on empty space");
        CHECK(!miss.entity.Valid(), "miss has no entity");
    }

    // --- 7. Raycast respects category/mask filtering ---
    // The query uses the default filter (category=0x1, mask=ALL), so a shape
    // whose own mask excludes category 0x1 is invisible to the ray.
    {
        Arc::Scene s;
        Arc::PhysicsWorld pw;
        pw.Create({0.0f, 0.0f});
        Arc::Entity ghost = s.CreateEntity("Ghost");
        ghost.GetComponent<Arc::TransformComponent>().Position = {0.0f, 0.0f, 0.0f};
        auto& grb = ghost.AddComponent<Arc::RigidBody2DComponent>();
        grb.Type = Arc::RigidBodyType::Static;
        grb.Category = 0x00000002u;
        grb.Mask = 0x00000002u; // excludes query category 0x1 => not hittable
        ghost.AddComponent<Arc::BoxCollider2DComponent>().Size = {1.0f, 1.0f};
        pw.AddBody(s, ghost);

        auto h = pw.Raycast(s, {0.0f, 5.0f}, {0.0f, -10.0f});
        CHECK(!h.hit, "raycast ignores body filtered out by mask");
    }

    // --- 8. Raycast safety: no world created => miss, no crash ---
    {
        Arc::Scene s;
        Arc::PhysicsWorld pw; // Create() never called
        auto h = pw.Raycast(s, {0.0f, 0.0f}, {1.0f, 1.0f});
        CHECK(!h.hit, "raycast on uncreated world is a safe miss");
    }

    std::printf("test_physics_filter: ALL OK\n");
    return 0;
}
