#pragma once
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Scene/Entity.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Arc {

class Scene;

// PhysicsWorld (P1 Step 2): thin C++ facade over vendored Box2D 3.x (C API).
// Bridges the ECS to a single b2World. Each entity with a RigidBody2DComponent
// gets one b2Body; a BoxCollider2DComponent adds a box shape to that body.
//
// Design notes:
//  * Box2D's C headers/types stay OUT of this public header (PIMPL) so gameplay
//    code never sees b2* symbols and only ArcEngine's .cpp links against box2d.
//  * Physics runs at a FIXED 1/60 step with an internal accumulator, so a
//    variable frame rate never perturbs the simulation (Box2D best practice).
//  * Transforms are synced both ways: Step() reads dynamic/kinematic body pose
//    back into TransformComponent (bodies drive rendering). SetTransform/velocity
//    helpers push gameplay intent into the sim.
//  * One world per PhysicsWorld instance; create via PhysicsWorld world; then
//    world.AddBody(scene, entity); ... world.Step(scene, dt).
class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // Create/destroy the underlying b2World. Default gravity is (0, -9.8) m/s^2.
    bool Create(const Vec2& gravity = {0.0f, -9.8f});
    void Destroy();

    // Build a Box2D body for an entity from its RigidBody2DComponent (+ optional
    // BoxCollider2DComponent) seeded at its TransformComponent. Idempotent per
    // entity (returns existing body if already added). Returns false if the
    // entity lacks a RigidBody2DComponent or the world isn't created.
    bool AddBody(Scene& scene, Entity e);
    bool RemoveBody(Entity e);
    bool HasBody(Entity e) const;

    // Compound bodies (P1 Step 9): runtime shape management. Both mutate the
    // entity's BoxCollider2DComponent::SubShapes (the source of truth, so the
    // change serializes) and — when the body already exists — the live Box2D
    // body immediately (mass recomputed). AddShape appends and returns the
    // new sub-shape index; RemoveShape destroys index `i` (false when the
    // entity has no collider, no such index, or the world isn't created —
    // the component edit still happens for AddShape when only the scene is
    // populated pre-AddBody).
    int AddShape(Scene& scene, Entity e, const SubShape2D& shape);
    bool RemoveShape(Scene& scene, Entity e, size_t index);

    // Joints (P1 Step 10): build the b2 joint described by e's
    // Joint2DComponent. Requires the world created and BOTH bodies already
    // added (target resolved via TargetId, else by TargetName lookup in the
    // scene). The target must be another entity — this Box2D build cannot
    // attach a joint side to the world (use a static anchor entity). Returns
    // true if the joint exists afterwards (idempotent); false when anything
    // is missing (component/body/target/creation failure).
    bool CreateJoint(Scene& scene, Entity e);
    // Destroy e's joint (false when none). RemoveBody first destroys any
    // joint referencing the entity on EITHER side, so ids never go stale.
    bool RemoveJoint(Entity e);

    // Advance the simulation by wall-clock dt (internally sub-stepped at 1/60)
    // and sync dynamic/kinematic body poses back into TransformComponents.
    void Step(Scene& scene, float dt);

    // --- gameplay intent / read-back (no-op if entity has no body) ---
    Vec2 GetLinearVelocity(Entity e) const;
    void SetLinearVelocity(Entity e, const Vec2& v);
    float GetAngularVelocity(Entity e) const;
    void ApplyImpulse(Entity e, const Vec2& impulse);
    void ApplyForce(Entity e, const Vec2& force);
    void Teleport(Entity e, const Vec2& position, float angleRadians = 0.0f);

    // Diagnostics.
    uint32_t BodyCount() const;
    void SetGravity(const Vec2& g);
    Vec2 GetGravity() const;

    // --- Collision events (P1 Step 4) ---
    // Box2D buffers begin/end touches for contacts (solid-solid) and sensors
    // (sensor-vs-anything) during b2World_Step. We drain those buffers after
    // every fixed sub-step into a raw id-pair buffer (never losing events when
    // one frame advances the sim multiple times) and resolve to Entity handles
    // at poll time. Each event is reported exactly once — poll right after
    // Step() each frame.
    //
    // Future-proofing contract (do not break — Area2D, netcode, replay rely on it):
    //  * Pairs are canonical: entityA.Id() <= entityB.Id(), so the same pair
    //    always compares equal regardless of Box2D's A/B wire order.
    //  * Poll is draining (exactly-once); SensorTouched is a non-consuming peek.
    //  * Stale ids are dropped at drain (destroyed b2 shapes/bodies) AND at poll
    //    (entities gone from the scene). RemoveBody also purges queued pairs.
    struct CollisionEvent {
        Entity entityA{};  // lower id of the pair (valid iff mapping held)
        Entity entityB{};  // higher id of the pair
        bool begun = true; // true = begin touch this step, false = end touch
        bool sensor = false; // true = sensor overlap event, false = solid contact
    };
    // All begin/end touches since the last call (copy, canonical order).
    std::vector<CollisionEvent> PollCollisionEvents(Scene& scene);
    // Peek (does NOT consume): did a *sensor* owned by `sensorEntity` begin
    // touching `otherEntity` since the last drain? Order-independent.
    bool SensorTouched(Scene& scene, Entity sensorEntity, Entity otherEntity) const;
    // Raycast the world from `origin` toward `translation` (a direction*length),
    // returning the closest hit as an ECS-friendly result. `hit` is false when
    // nothing was hit. This is a pure query — it never moves anything, so it is
    // safe to call mid-step. `scene` is used only to wrap the hit body's stored
    // entity id into a valid Entity handle.
    struct RaycastHit {
        bool hit = false;
        Entity entity{};       // entity that was hit (invalid if hit==false)
        Vec2 point{0, 0};      // world-space impact point
        Vec2 normal{0, 0};     // surface normal at the hit
        float fraction = 1.0f; // 0..1 along the input translation
    };
    RaycastHit Raycast(Scene& scene, const Vec2& origin, const Vec2& translation) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace Arc
