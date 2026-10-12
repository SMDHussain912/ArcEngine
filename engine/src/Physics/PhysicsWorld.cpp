#include "ArcEngine/Physics/PhysicsWorld.h"

#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Scene.h"

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Arc {

namespace {

// Raw begin/end touch drained after every fixed sub-step (Box2D's contact and
// sensor buffers reset per b2World_Step, so per-substep draining is the only
// way multi-substep frames keep every event). Poll resolves ids to Entity
// handles + clears the buffer.
struct RawEvent {
    uint32_t idA = 0;
    uint32_t idB = 0;
    bool began = true; // true = begin touch, false = end touch
    bool sensor = false; // true = sensor overlap, false = solid contact
};

} // namespace

// All Box2D C types are confined to this translation unit via the PIMPL Impl.
struct PhysicsWorld::Impl {
    b2WorldId world = {};        // index1 == 0 => not created
    float accumulator = 0.0f;   // leftover wall-clock time (fixed-step carry)
    bool created = false;
    // entity id -> Box2D body handle. One body per rigid entity.
    std::unordered_map<uint32_t, b2BodyId> bodies;
    // entity id -> compound sub-shape ids (P1 Step 9), index-aligned with
    // BoxCollider2DComponent::SubShapes; null ids keep alignment when a
    // degenerate sub-shape was skipped.
    std::unordered_map<uint32_t, std::vector<b2ShapeId>> subShapeIds;
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
        m_impl->subShapeIds.clear();
        m_impl->events.clear();
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

// Drain one sub-step's begin/end touches into the raw buffer. Must run after
// EVERY b2World_Step (Box2D clears its transient buffers per step, so events
// from intermediate sub-steps would otherwise be lost). Pairs are stored in
// canonical id order (min first) so consumers compare stably. Stale shape ids
// from bodies destroyed mid-frame are dropped via b2Shape_IsValid.
static void DrainEvents(b2WorldId world, std::vector<RawEvent>& out) {
    // shape -> owning body -> stored entity id. Guards: destroyed shapes/bodies
    // are dropped here (b2 docs warn event ids go stale on destroy).
    auto entityOf = [](b2ShapeId sid) -> uint32_t {
        if (!b2Shape_IsValid(sid)) return 0;
        b2BodyId b = b2Shape_GetBody(sid);
        if (!b2Body_IsValid(b)) return 0;
        uintptr_t id = reinterpret_cast<uintptr_t>(b2Body_GetUserData(b));
        return static_cast<uint32_t>(id);
    };
    auto push = [&out](uint32_t a, uint32_t b, bool began, bool sensor) {
        if (a == 0 || b == 0) return;           // unmapped/stale side — drop
        if (a == b) return;                     // self-touch (same entity) — drop
        if (b < a) out.push_back({b, a, began, sensor});
        else out.push_back({a, b, began, sensor});
    };

    b2ContactEvents ce = b2World_GetContactEvents(world);
    for (int i = 0; i < ce.beginCount; ++i)
        push(entityOf(ce.beginEvents[i].shapeIdA), entityOf(ce.beginEvents[i].shapeIdB),
             true, false);
    for (int i = 0; i < ce.endCount; ++i)
        push(entityOf(ce.endEvents[i].shapeIdA), entityOf(ce.endEvents[i].shapeIdB),
             false, false);

    b2SensorEvents se = b2World_GetSensorEvents(world);
    for (int i = 0; i < se.beginCount; ++i)
        push(entityOf(se.beginEvents[i].sensorShapeId), entityOf(se.beginEvents[i].visitorShapeId),
             true, true);
    for (int i = 0; i < se.endCount; ++i)
        push(entityOf(se.endEvents[i].sensorShapeId), entityOf(se.endEvents[i].visitorShapeId),
             false, true);
}

// Create one compound sub-shape (P1 Step 9) at a body-local offset with its
// OWN material/sensor flag and the body's collision filter. Returns a null
// shape id (with the caller warning) on degenerate geometry.
static b2ShapeId CreateSubShape(b2BodyId body, const SubShape2D& ss,
                                const RigidBody2DComponent& rb) {
    b2ShapeDef sd = b2DefaultShapeDef();
    sd.density = ss.Density;
    sd.material.friction = ss.Friction;
    sd.material.restitution = ss.Restitution;
    sd.isSensor = ss.IsSensor;
    sd.enableContactEvents = !ss.IsSensor;
    sd.enableSensorEvents = true;
    sd.filter.categoryBits = rb.Category;
    sd.filter.maskBits = rb.Mask;
    sd.filter.groupIndex = rb.GroupIndex;
    switch (ss.Shape) {
        case SubShape2D::Type::Circle: {
            if (ss.Radius <= 0.0f) return b2ShapeId{};
            b2Circle c = {{ss.Offset.x, ss.Offset.y}, ss.Radius};
            return b2CreateCircleShape(body, &sd, &c);
        }
        case SubShape2D::Type::Capsule: {
            if (ss.Radius <= 0.0f) return b2ShapeId{};
            b2Capsule cap;
            cap.center1 = {ss.Offset.x, ss.Offset.y - ss.Height * 0.5f};
            cap.center2 = {ss.Offset.x, ss.Offset.y + ss.Height * 0.5f};
            cap.radius = ss.Radius;
            return b2CreateCapsuleShape(body, &sd, &cap);
        }
        case SubShape2D::Type::Box:
        default: {
            b2Polygon box = b2MakeOffsetBox(ss.Size.x * 0.5f, ss.Size.y * 0.5f,
                                            {ss.Offset.x, ss.Offset.y},
                                            b2MakeRot(0.0f));
            return b2CreatePolygonShape(body, &sd, &box);
        }
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
        // Shape selection (P1 Steps 5-8), most-specific first:
        //   Chain.size() >= 2 -> one-sided chain terrain (b2CreateChain)
        //   VertexCount >= 3  -> convex polygon (hull-validated; falls back to
        //                        box with an error log if b2ComputeHull fails)
        //   Height > 0        -> capsule (vertical segment + Radius caps)
        //   Radius  > 0       -> circle
        //   otherwise         -> box (half-extents from Size)
        if (col.Chain.size() >= 2) {
            // Chain (P1 Step 8): one-sided terrain — collision only on the
            // RIGHT side of the point order (b2RightPerp side test), so
            // walkable ground winds right-to-left unless ChainFlip reverses.
            if (col.IsSensor)
                Arc::Log::Warn("PhysicsWorld: IsSensor is not supported on chain "
                               "shapes (entity " + std::to_string(e.Id()) +
                               ") — chain will be solid");
            const size_t n = col.Chain.size();
            const bool loop = col.ChainLoop;
            // Box2D debug-asserts any segment/ghost pair <= B2_LINEAR_SLOP, so
            // pre-validate here: bad input degrades to a box instead of
            // aborting a debug build.
            bool ok = !loop || n >= 3;
            const size_t segs = loop ? n : n - 1;
            for (size_t i = 0; i < segs && ok; ++i) {
                const Vec2& a = col.Chain[i];
                const Vec2& b = col.Chain[(i + 1) % n];
                float dx = b.x - a.x, dy = b.y - a.y;
                if (dx * dx + dy * dy <= B2_LINEAR_SLOP * B2_LINEAR_SLOP)
                    ok = false;
            }
            std::vector<b2Vec2> pts(n);
            for (size_t i = 0; i < n; ++i) {
                const Vec2& v = col.Chain[col.ChainFlip ? (n - 1 - i) : i];
                pts[i] = {v.x, v.y};
            }
            b2ChainId chain{};
            if (ok) {
                b2ChainDef cd = b2DefaultChainDef();
                cd.points = pts.data();
                cd.pointCount = static_cast<int>(n);
                cd.isLoop = loop;
                if (!loop) {
                    // Extrapolated ghost ends (standard practice): ghost-to-
                    // end distance equals the adjacent segment length.
                    cd.ghost1 = {2.0f * pts[0].x - pts[1].x,
                                 2.0f * pts[0].y - pts[1].y};
                    const size_t m = n - 1;
                    cd.ghost2 = {2.0f * pts[m].x - pts[m - 1].x,
                                 2.0f * pts[m].y - pts[m - 1].y};
                }
                b2SurfaceMaterial mat{};
                mat.friction = col.Friction;
                mat.restitution = col.Restitution;
                cd.materials = &mat;
                cd.materialCount = 1;
                cd.filter.categoryBits = rb.Category;
                cd.filter.maskBits = rb.Mask;
                cd.filter.groupIndex = rb.GroupIndex;
                cd.enableSensorEvents = true;
                chain = b2CreateChain(body, &cd);
            }
            if (b2Chain_IsValid(chain)) {
                // b2CreateChain hard-codes contact events OFF per segment;
                // re-enable so PollCollisionEvents treats chain ground like
                // every other shape (P1 Step 4 parity).
                int segCount = b2Chain_GetSegmentCount(chain);
                std::vector<b2ShapeId> shapes(static_cast<size_t>(segCount));
                int got = b2Chain_GetSegments(chain, shapes.data(), segCount);
                for (int i = 0; i < got; ++i)
                    b2Shape_EnableContactEvents(shapes[i], true);
            } else {
                Arc::Log::Error("PhysicsWorld: invalid chain on entity " +
                                std::to_string(e.Id()) + " (too few points, " +
                                "segment shorter than slop, or bad loop) — " +
                                "falling back to box");
                b2Polygon box = b2MakeBox(col.Size.x * 0.5f, col.Size.y * 0.5f);
                b2CreatePolygonShape(body, &sd, &box);
            }
        } else if (col.VertexCount >= 3) {
            b2Vec2 pts[8];
            int n = col.VertexCount > 8 ? 8 : col.VertexCount;
            for (int i = 0; i < n; ++i)
                pts[i] = {col.Vertices[i].x, col.Vertices[i].y};
            b2Hull hull = b2ComputeHull(pts, n);
            if (hull.count >= 3) {
                b2Polygon poly = b2MakePolygon(&hull, 0.0f);
                b2CreatePolygonShape(body, &sd, &poly);
            } else {
                Arc::Log::Error("PhysicsWorld: degenerate polygon on entity " +
                                std::to_string(e.Id()) + " (collinear/insufficient " +
                                "points) — falling back to box");
                b2Polygon box = b2MakeBox(col.Size.x * 0.5f, col.Size.y * 0.5f);
                b2CreatePolygonShape(body, &sd, &box);
            }
        } else if (col.Height > 0.0f) {
            b2Capsule cap;
            cap.center1 = {0.0f, -col.Height * 0.5f};
            cap.center2 = {0.0f, col.Height * 0.5f};
            cap.radius = col.Radius;
            b2CreateCapsuleShape(body, &sd, &cap);
        } else if (col.Radius > 0.0f) {
            b2Circle circle = {b2Vec2_zero, col.Radius};
            b2CreateCircleShape(body, &sd, &circle);
        } else {
            b2Polygon box = b2MakeBox(col.Size.x * 0.5f, col.Size.y * 0.5f);
            b2CreatePolygonShape(body, &sd, &box);
        }
    }

    // Compound sub-shapes (P1 Step 9): one b2 shape per entry, ids kept
    // index-aligned with the component (null id = skipped degenerate entry).
    if (scene.HasComponent<BoxCollider2DComponent>(e.Id())) {
        const auto& col = scene.GetComponent<BoxCollider2DComponent>(e.Id());
        auto& ids = m_impl->subShapeIds[e.Id()];
        ids.reserve(col.SubShapes.size());
        for (size_t i = 0; i < col.SubShapes.size(); ++i) {
            b2ShapeId sid = CreateSubShape(body, col.SubShapes[i], rb);
            if (!b2Shape_IsValid(sid))
                Arc::Log::Warn("PhysicsWorld: sub-shape " + std::to_string(i) +
                               " on entity " + std::to_string(e.Id()) +
                               " degenerate (radius<=0) — skipped");
            ids.push_back(sid);
        }
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
    m_impl->subShapeIds.erase(e.Id()); // ids die with the body
    // Purge queued pairs mentioning this entity: Box2D emits an end-touch on
    // destroy, which we may already have drained — either way no stale pair
    // may survive to mislead the next poll (Area2D/pickup correctness).
    auto& evs = m_impl->events;
    evs.erase(std::remove_if(evs.begin(), evs.end(), [&](const RawEvent& ev) {
                  return ev.idA == e.Id() || ev.idB == e.Id();
              }),
              evs.end());
    return true;
}

bool PhysicsWorld::HasBody(Entity e) const {
    return e.Valid() && m_impl->bodies.find(e.Id()) != m_impl->bodies.end();
}

int PhysicsWorld::AddShape(Scene& scene, Entity e, const SubShape2D& shape) {
    if (!e.Valid() || !scene.HasComponent<BoxCollider2DComponent>(e.Id()))
        return -1;
    auto& col = scene.GetComponent<BoxCollider2DComponent>(e.Id());
    col.SubShapes.push_back(shape);
    const int index = static_cast<int>(col.SubShapes.size()) - 1;
    // Live body: create the b2 shape now so the change lands this frame and
    // the body's mass is recomputed by Box2D.
    auto it = m_impl->bodies.find(e.Id());
    if (m_impl->created && it != m_impl->bodies.end() &&
        scene.HasComponent<RigidBody2DComponent>(e.Id())) {
        const auto& rb = scene.GetComponent<RigidBody2DComponent>(e.Id());
        b2ShapeId sid = CreateSubShape(it->second, shape, rb);
        if (!b2Shape_IsValid(sid))
            Arc::Log::Warn("PhysicsWorld: AddShape degenerate (radius<=0) on " +
                           std::to_string(e.Id()) + " — kept in component only");
        m_impl->subShapeIds[e.Id()].push_back(sid); // keep index alignment
        b2Body_SetAwake(it->second, true); // sleeping bodies must react
    }
    return index;
}

bool PhysicsWorld::RemoveShape(Scene& scene, Entity e, size_t index) {
    if (!e.Valid() || !scene.HasComponent<BoxCollider2DComponent>(e.Id()))
        return false;
    auto& col = scene.GetComponent<BoxCollider2DComponent>(e.Id());
    if (index >= col.SubShapes.size()) return false;
    auto idIt = m_impl->subShapeIds.find(e.Id());
    if (idIt != m_impl->subShapeIds.end() && index < idIt->second.size()) {
        b2ShapeId sid = idIt->second[index];
        if (b2Shape_IsValid(sid))
            b2DestroyShape(sid, true); // true: recompute body mass
        idIt->second.erase(idIt->second.begin() + static_cast<long>(index));
        auto bodyIt = m_impl->bodies.find(e.Id());
        if (bodyIt != m_impl->bodies.end())
            b2Body_SetAwake(bodyIt->second, true); // must re-settle
    }
    col.SubShapes.erase(col.SubShapes.begin() + static_cast<long>(index));
    return true;
}

void PhysicsWorld::Step(Scene& scene, float dt) {
    if (!m_impl->created) return;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > Impl::kMaxFrameTime) dt = Impl::kMaxFrameTime; // clamp huge hitches

    // Fixed-step accumulation: variable frame rate never perturbs the sim.
    m_impl->accumulator += dt;
    while (m_impl->accumulator >= Impl::kFixedStep) {
        b2World_Step(m_impl->world, Impl::kFixedStep, Impl::kSubSteps);
        DrainEvents(m_impl->world, m_impl->events); // buffers reset per step!
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

std::vector<PhysicsWorld::CollisionEvent> PhysicsWorld::PollCollisionEvents(Scene& scene) {
    std::vector<CollisionEvent> out;
    if (!m_impl->created) return out;

    // Live-entity set: resolves raw ids to handles, dropping stale ids from
    // entities destroyed since the event fired (Box2D lets shapes go stale).
    std::unordered_set<uint32_t> live;
    for (const Entity& e : scene.Entities())
        live.insert(e.Id());

    out.reserve(m_impl->events.size());
    for (const auto& raw : m_impl->events) {
        if (live.find(raw.idA) == live.end() || live.find(raw.idB) == live.end())
            continue; // participant gone — skip stale pair
        CollisionEvent ev;
        ev.entityA = Entity(&scene, raw.idA);
        ev.entityB = Entity(&scene, raw.idB);
        ev.begun = raw.began;
        ev.sensor = raw.sensor;
        out.push_back(ev);
    }
    m_impl->events.clear(); // drained exactly once, like Box2D's buffers
    return out;
}

bool PhysicsWorld::SensorTouched(Scene& scene, Entity sensorEntity, Entity otherEntity) const {
    if (!m_impl->created || !sensorEntity.Valid() || !otherEntity.Valid()) return false;
    if (sensorEntity.Id() == otherEntity.Id()) return false; // self never counts
    for (const auto& raw : m_impl->events) {
        if (!raw.sensor || !raw.began) continue;
        // Buffer is canonical (min first) — normalize the query the same way.
        uint32_t lo = sensorEntity.Id(), hi = otherEntity.Id();
        if (hi < lo) std::swap(lo, hi);
        if (raw.idA == lo && raw.idB == hi) return true;
    }
    (void)scene; // ids resolve through PhysicsWorld's own bookkeeping; scene
                 // kept for API symmetry with PollCollisionEvents.
    return false;
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
