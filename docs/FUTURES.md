# ArcEngine FUTURES — every feature we still need, in build order.
> Living tracker. Check `[x]` when done, keep one line per item, link the test/example.
> Source of truth for phases: `docs/V1_BLUEPRINT.md`. History: `docs/ROADMAP.md`.
> Rule: after every step, push to `origin/dev` (bro rule).

## Legend
- `[ ]` todo · `[~]` in progress · `[x]` done
- Each item: what + where it lives + how we verify.

## P1 — Shippable 2D (current phase)
### Physics 2D — Box2D v3 (PhysicsWorld facade, PIMPL, fixed 1/60)
- [x] Step 2 Base: RigidBody2D (Static/Kinematic/Dynamic, FixedRotation, GravityScale, damping) + BoxCollider2D (Size/Density/Friction/Restitution/IsSensor) — `engine/*/Physics/PhysicsWorld.*`, `tests/test_physics.cpp`, `examples/08-physics`
- [x] Step 3 Filtering + raycast: Category/Mask/GroupIndex → b2Filter, entity id in body userData, Raycast → RaycastHit — `tests/test_physics_filter.cpp`
- [x] Editor: Inspector UI + add/remove + viewport collider outlines + stepping only in Play — `engine/src/Editor/ArcEditor.cpp`
- [x] .arc: rigidbody2d + boxcollider2d + category/mask/group_index round-trip — `tests/test_physics_serialize.cpp`
- [~] Step 4 Collision events (IN PROGRESS): header declares CollisionEvent + PollCollisionEvents + SensorTouched; left: finish DrainEvents wiring in Step(), RemoveBody/Destroy guards, test_physics_events.cpp
- [ ] Step 5 CircleCollider2D: radius + offset shape, .arc round-trip, viewport circle outline
- [ ] Step 6 Joints: RevoluteJoint2D + DistanceJoint2D components (anchorA/anchorB, limits, motor lite), .arc, test
- [ ] Step 7 CharacterBody2D: kinematic move-and-slide helper (velocity, snap, floor normal), test on slopes/steps
- [ ] Step 8 Area2D triggers: sensor-only body + OnAreaEnter/Exit via events (coin pickup path)
- [ ] Step 9 Debug draw: polygon/circle outlines + contact points in viewport (toggle)
- [ ] P1 demo: `projects/demo_2d/` platformer .arc (player + platforms + coins + blip) + `examples/09-*`; accept: 60fps collisions on Intel HD 620

### Rendering 2D
- [x] SpriteComponent + SpriteBatch (layer+texture sort, 1 draw/bind) — `examples/07-sprites`
- [ ] Camera2D: ortho + zoom + active stack, follow/limits lite
- [ ] TextureRegion/atlas: packed atlas + region lookup, canvas sort (z + layer)

### Scripting 2D API (needs P5 Lua first)
- [ ] transform2d, Input.axis/action, Audio.play, Physics.raycast2d; ready/process/physics_process

## P0 — Harden (foundation leftovers)
- [ ] OpenAL replaces miniaudio: AudioServer (Listener/Source/Buffer, Master→SFX→Music buses), WAV own-decode + OGG stb_vorbis, 2D distance volume; keep miniaudio note as docs/legacy-miniaudio.md
- [ ] AssetManager already done; add: model importer hook (cgltf later), .arc scene import path
- [ ] CI: cmake configure+build on Linux (GitHub Actions later)

## P2 — Shippable 3D
- [ ] Camera3D (perspective + orbit helper)
- [ ] Material (albedo/roughness/metallic, dir+point+spot, forward pass) + real light shading hookup
- [ ] Model via cgltf (mesh + materials + nodes) — vendor third_party/cgltf
- [ ] PhysicsServer3D over Jolt: RigidBody3D, StaticBody3D, CharacterBody3D, layers, raycast — vendor third_party/jolt
- [ ] Lua 3D API: transform3d, camera helpers, spawn_model
- [ ] examples/09-model3d: lit glTF room + falling crates + orbit camera
- [ ] projects/demo_3d/ playground .arc; accept: lit 3D + rigid bodies + orbit cam, GLES-compatible shaders

## P3 — Editor (visual building)
- [x] Done: ImGui Hierarchy/Inspector/Viewport/Assets/Console, Play/Stop, gizmos, undo/redo
- [ ] Assets browser panel (real file view + import status)
- [ ] Lua hot-reload on save in Play mode + scene dirty tracking
- [ ] tools/arc_import: CLI reimport changed assets (hash check)
- [ ] Accept: build + play demo_2d without editing C++

## P4 — Mobile (Android)
- [ ] OpenGLESRenderer parity: #version 320 es shader variants, ETC2/ASTC notes, perf budget
- [ ] Window EGL backend (NativeActivity), touch Input (multi-touch, gestures)
- [ ] AudioServer via OpenSL ES / AAudio path
- [ ] android/ template + Gradle, APK export from editor; accept: demo_2d + demo_3d on phone

## P5 — Scripting complete
- [ ] Vendor third_party/lua (5.4) + third_party/sol2/single/sol.hpp
- [ ] engine/script/ScriptSystem owning sol::state; lifecycle Awake/Start/Update(dt)/OnDestroy, hot-reload
- [ ] Full Lua API (Scene/Physics/Audio/Assets), Log + error overlays, debugger hooks
- [ ] Sandbox profiles: game (restricted os/io) vs editor (full)
- [ ] docs/LUA_API.md generated from bindings + examples; accept: Lua-only 2D mini-game, zero recompile
- [ ] Visual nodes: scope + prototype AFTER V1 ships (Lua stays primary)

## V1.1+ (deliberately deferred — do NOT build in V1)
- Networking, navmesh/pathfinding, skeletal animation retargeting, TileMap autotiling, particles v2, consoles, visual scripting v1.

## Next up (agreed order)
1. Finish Step 4 collision events (drain wiring + test + push).
2. Step 5 CircleCollider2D.
3. Step 6 Joints (revolute + distance).
