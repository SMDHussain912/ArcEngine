#include "ArcEngine/Physics/PhysicsWorld.h"

#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include <box2d/box2d.h>

#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Arc {

// All Box2D C types are confined to this translation unit via the PIMPL Impl.
struct PhysicsWorld::Impl {
    b2WorldId world = {};        // index1 == 0 => not created
    float accumulator = 0.0f;   // leftover wall-clock time (fixed-step carry)
    bool created = false;
    // entity id -> Box2D body handle. One body per rigid entity.
    std::unordered_map<uint32_t, b2BodyId> bodies;

    // Raw begin/end touches drained after every fixed sub-step (Box2D's event
    // buffers reset per b2World_Step, so buffering here survives multi-substep
    // frames). Entity ids are resolved to Handles at poll time so stale ids
    // from destroyed bodies are dropped when the scene no longer lists them.
    struct RawEvent {
        uint32_t a = 0;
        uint32_t b = 0;
        bool begun = true;
        bool sensor = false;
    };
    std::vector<RawEvent> events;

    static constexpr float kFixedStep = 1.0f / 60.0f; // Box2D tuned step
    static constexpr int kSubSteps = 4;                // recommended solver iterations
    static constexpr float kMaxFrameTime = 0.25f;      // clamp: avoid catch-up spiral
};

PhysicsWorld::PhysicsWorld() : m_impl(new Impl) {}

PhysicsWorld::~PhysicsWorld() {
    Destroy();
}

bool PhysicsWorld::Create(const Vec2& gravity) {
    if (m_impl->created) return true; // idempotent

    b2WorldDef def = b2DefaultWorldDef();
    def.gravity = {gravity.x, gravity.y};
    def.workerCount = 1; // single-threaded => deterministic, no task-system needed

    m_impl->world = b2CreateWorld(&def);
    m_impl->created = b2World_IsValid(m_impl->world);
    if (!m_impl->created)
        Arc::Log::Error("PhysicsWorld: b2CreateWorld failed");
    return m_impl->created;
}

void PhysicsWorld::Destroy() {
    if (m_impl->created) {
        b2DestroyWorld(m_impl->world); // destroys all bodies it owns
        m_impl->world = {};
        m_impl->bodies.clear();
        m_impl->accumulator = 0.0f;
        m_impl->created = false;
    }
}

static b2BodyType ToB2BodyType(RigidBodyType t) {
    switch (t) {
        case RigidBodyType::Static: return b2_staticBody;
        case RigidBodyType::Kinematic: return b2_kinematicBody;
        case RigidBodyType::Dynamic:
        default: return b2_dynamicBody;
    }
}

bool PhysicsWorld::AddBody(Scene& scene, Entity e) {
    if (!m_impl->created || !e.Valid() || !scene.HasComponent<RigidBody2DComponent>(e.Id()))
        return false;
    if (m_impl->bodies.find(e.Id()) != m_impl->bodies.end())
        return true; // already present — idempotent

    const auto& tc = scene.GetComponent<TransformComponent>(e.Id());
    const auto& rb = scene.GetComponent<RigidBody2DComponent>(e.Id());

    b2BodyDef bd = b2DefaultBodyDef();
    bd.type = ToB2BodyType(rb.Type);
    bd.position = {tc.Position.x, tc.Position.y};
    bd.rotation = b2MakeRot(tc.Rotation.z); // euler Z (radians) -> Box2D rot
    bd.gravityScale = rb.GravityScale;
    bd.linearDamping = rb.LinearDamping;
    bd.angularDamping = rb.AngularDamping;
    bd.motionLocks.angularZ = rb.FixedRotation;

    b2BodyId body = b2CreateBody(m_impl->world, &bd);
    if (!b2Body_IsValid(body)) {
        Arc::Log::Error("PhysicsWorld: b2CreateBody failed for entity " + std::to_string(e.Id()));
        return false;
    }

    // Stash the owning entity id on the body so queries (e.g. Raycast) can map
    // a b2ShapeId -> body -> entity without a reverse lookup table.
    b2Body_SetUserData(body, reinterpret_cast<void*>(static_cast<uintptr_t>(e.Id())));

    // Optional box collider (full size -> half extents).
    if (scene.HasComponent<BoxCollider2DComponent>(e.Id())) {
        const auto& col = scene.GetComponent<BoxCollider2DComponent>(e.Id());
        b2ShapeDef sd = b2DefaultShapeDef();
        sd.density = col.Density;
        sd.material.friction = col.Friction;
        sd.material.restitution = col.Restitution;
        sd.isSensor = col.IsSensor;
        // Events (P1 Step 4): contact touches apply to solid shapes on simulated
        // bodies; sensor touches need the flag on BOTH shapes, so set it always
        // (cheap per-pair bool test, zero false traffic).
        sd.enableContactEvents = !col.IsSensor;
        sd.enableSensorEvents = true;
        // Collision filtering: which layers this shape is, and which it hits.
        sd.filter.categoryBits = rb.Category;
        sd.filter.maskBits = rb.Mask;
        sd.filter.groupIndex = rb.GroupIndex;
        b2Polygon box = b2MakeBox(col.Size.x * 0.5f, col.Size.y * 0.5f);
        b2CreatePolygonShape(body, &sd, &box);
    }

    m_impl->bodies[e.Id()] = body;
    return true;
}

bool PhysicsWorld::RemoveBody(Entity e) {
    if (!e.Valid()) return false;
    auto it = m_impl->bodies.find(e.Id());
    if (it == m_impl->bodies.end()) return false;
    b2DestroyBody(it->second);
    m_impl->bodies.erase(it);
    return true;
}

bool PhysicsWorld::HasBody(Entity e) const {
    return e.Valid() && m_impl->bodies.find(e.Id()) != m_impl->bodies.end();
}

void PhysicsWorld::Step(Scene& scene, float dt) {
    if (!m_impl->created) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > Impl::kMaxFrameTime) dt = Impl::kMaxFrameTime; // clamp huge hitches

    // Fixed-step accumulation: variable frame rate never perturbs the sim.
    m_impl->accumulator += dt;
    while (m_impl->accumulator >= Impl::kFixedStep) {
        b2World_Step(m_impl->world, Impl::kFixedStep, Impl::kSubSteps);
        m_impl->accumulator -= Impl::kFixedStep;
    }

    // Sync simulated poses back into transforms (bodies drive rendering).
    // Static bodies are authoritative from the component side, so we skip them.
    for (const auto& [id, body] : m_impl->bodies) {
        if (b2Body_GetType(body) == b2_staticBody) continue;
        if (!scene.HasComponent<TransformComponent>(id)) continue;
        b2Pos p = b2Body_GetPosition(body);
        float angle = b2Rot_GetAngle(b2Body_GetRotation(body));
        auto& tc = scene.GetComponent<TransformComponent>(id);
        tc.Position.x = p.x;
        tc.Position.y = p.y;
        tc.Rotation.z = angle;
    }
}

Vec2 PhysicsWorld::GetLinearVelocity(Entity e) const {
    auto it = m_impl->bodies.find(e.Valid() ? e.Id() : 0);
    if (it == m_impl->bodies.end()) return {0.0f, 0.0f};
    b2Vec2 v = b2Body_GetLinearVelocity(it->second);
    return {v.x, v.y};
}

void PhysicsWorld::SetLinearVelocity(Entity e, const Vec2& v) {
    if (!e.Valid()) return;
    auto it = m_impl->bodies.find(e.Id());
    if (it == m_impl->bodies.end()) return;
    b2Body_SetLinearVelocity(it->second, {v.x, v.y});
}

float PhysicsWorld::GetAngularVelocity(Entity e) const {
    auto it = m_impl->bodies.find(e.Valid() ? e.Id() : 0);
    if (it == m_impl->bodies.end()) return 0.0f;
    return b2Body_GetAngularVelocity(it->second);
}

void PhysicsWorld::ApplyImpulse(Entity e, const Vec2& impulse) {
    if (!e.Valid()) return;
    auto it = m_impl->bodies.find(e.Id());
    if (it == m_impl->bodies.end()) return;
    b2Body_ApplyLinearImpulseToCenter(it->second, {impulse.x, impulse.y}, true);
}

void PhysicsWorld::ApplyForce(Entity e, const Vec2& force) {
    if (!e.Valid()) return;
    auto it = m_impl->bodies.find(e.Id());
    if (it == m_impl->bodies.end()) return;
    b2Body_ApplyForceToCenter(it->second, {force.x, force.y}, true);
}

void PhysicsWorld::Teleport(Entity e, const Vec2& position, float angleRadians) {
    if (!e.Valid()) return;
    auto it = m_impl->bodies.find(e.Id());
    if (it == m_impl->bodies.end()) return;
    b2Body_SetTransform(it->second, {position.x, position.y}, b2MakeRot(angleRadians));
}

uint32_t PhysicsWorld::BodyCount() const {
    return static_cast<uint32_t>(m_impl->bodies.size());
}

void PhysicsWorld::SetGravity(const Vec2& g) {
    if (!m_impl->created) return;
    b2World_SetGravity(m_impl->world, {g.x, g.y});
}

Vec2 PhysicsWorld::GetGravity() const {
    if (!m_impl->created) return {0.0f, 0.0f};
    b2Vec2 g = b2World_GetGravity(m_impl->world);
    return {g.x, g.y};
}

PhysicsWorld::RaycastHit PhysicsWorld::Raycast(Scene& scene, const Vec2& origin,
                                               const Vec2& translation) const {
    RaycastHit out;
    if (!m_impl->created) return out;

    // Query filter that accepts every category (a raw raycast; per-shape filter
    // bits still gate the hit via Box2D's own category/mask test).
    b2QueryFilter qf = b2DefaultQueryFilter();

    b2RayResult r = b2World_CastRayClosest(m_impl->world, {origin.x, origin.y},
                                           {translation.x, translation.y}, qf);
    if (!r.hit) return out;

    out.hit = true;
    out.point = {r.point.x, r.point.y};
    out.normal = {r.normal.x, r.normal.y};
    out.fraction = r.fraction;

    // Map the hit shape -> owning body -> stored entity id.
    b2BodyId hitBody = b2Shape_GetBody(r.shapeId);
    if (b2Body_IsValid(hitBody)) {
        uintptr_t id = reinterpret_cast<uintptr_t>(b2Body_GetUserData(hitBody));
        uint32_t eid = static_cast<uint32_t>(id);
        // Only wrap an entity we still track (guards stale bodies / id reuse).
        if (eid != 0 && m_impl->bodies.find(eid) != m_impl->bodies.end())
            out.entity = Entity(&scene, eid);
    }
    return out;
}

} // namespace Arc
