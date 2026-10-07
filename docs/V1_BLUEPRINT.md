# ArcEngine V1 Blueprint — Production 2D + 3D Engine

> Companion to `README.md` (front page) and `ARCHITECTURE.md` (deep design).
> Prototype log M0–M6 lives in `ROADMAP.md`. This file is the build plan.

**Locked decisions:** Unity-style Entities · Godot-inspired Servers · Lua now, visual later ·
OpenAL now (replace miniaudio) · Box2D v3 (2D) + Jolt (3D) · OpenGL 4.6 → GLES 3.2.

## Phase P0 — Harden (foundation)

Goal: turn prototype into production base. No new gameplay yet.

- [ ] `Core/App`: fixed-step loop (physics 60Hz accumulator, process, render),
      `project.arc` load, graceful shutdown order (Script→Physics→Audio→Render)
- [ ] `Assets/AssetManager`: UUID registry, `assets/*.arc.import` cache,
      sync loaders (async in P3), `File::SearchPaths`
- [ ] `.arc` scene format (YAML): `Scene > Entities > Components` + load/save round-trip
- [ ] `Audio/AudioServer` over **OpenAL**: device/context, Listener/Source/Buffer,
      `AudioPlayer2D/3D`, master/SFX/music buses; decode WAV (own) + OGG (stb_vorbis)
- [ ] Remove `Audio` miniaudio class → keep file as `docs/legacy-miniaudio.md` note
- [ ] `tests/`: headless `Scene save→load`, `AssetManager` round-trip
- [ ] CI: `cmake -B build && cmake --build` on Linux (GitHub Actions later)

**Accept:** `ArcRuntime projects/empty/project.arc` boots, plays OpenAL blip on Space.
**Tag:** `v1.0-p0`

## Phase P1 — Shippable 2D

- [ ] `Rendering/Camera2D` (ortho, zoom, active stack), `Rendering/Sprite2D`
      (batched quads, texture atlas, `TextureRegion`), canvas sort (z + layer)
- [ ] `Physics/PhysicsServer2D` over Box2D v3: `RigidBody2D`, `StaticBody2D`,
      `CharacterBody2D`, `Area2D`, collision layers/masks, raycast
## Phase P2 — Shippable 3D

- [ ] `Rendering/Camera3D` (perspective + orbit helper), `Rendering/Material`
      (albedo/roughness/metallic, dir+point+spot lights, forward pass),
      `Rendering/Model` via cgltf (mesh + materials + nodes)
- [ ] `Physics/PhysicsServer3D` over Jolt: `RigidBody3D`, `StaticBody3D`,
      `CharacterBody3D`, layers, raycast
- [ ] Lua 3D API: `transform3d`, camera helpers, `spawn_model`
- [ ] `examples/08-model3d`: lit glTF room + falling crates + orbit camera
- [ ] `projects/demo_3d/`: small 3D playground in `.arc`

**Accept:** lit 3D + rigid bodies + orbit camera, GLES-shader compatible.
**Tag:** `v1.0-p2`

## Phase P3 — Editor

- [ ] `editor/ArcEditor` (ImGui docking): Hierarchy, Inspector (Transform/Sprite/Material/Audio),
      Viewport 2D/3D (gizmos: move/rotate/scale), Assets browser, Console, toolbar Play/Stop
- [ ] Play mode: runs `App` in-process, Lua hot-reload on save, scene dirty tracking
- [ ] `tools/arc_import`: CLI reimport changed assets (hash check)

**Accept:** build + play `demo_2d` without editing C++.
## Phase P4 — Mobile (Android)

- [ ] `Rendering/OpenGLESRenderer` parity: `#version 320 es` shader variants,
      ETC2/ASTC texture notes, perf budget (draw calls, overdraw)
- [ ] `Core/Window` EGL backend (`NativeActivity`), touch `Input` (multi-touch,
      gestures), accelerometer (later), APK `android/` template + Gradle
- [ ] `AudioServer` via OpenSL ES / AAudio path (OpenAL-Soft Android backend)
- [ ] Export from editor: `project.arc` + packed assets → APK

**Accept:** `demo_2d` + `demo_3d` run on physical phone at acceptable fps.
**Tag:** `v1.0-p4`

## Phase P5 — Scripting complete

- [ ] Full Lua API coverage (Scene/Physics/Audio/Assets), `Log` + error overlays,
      Lua debugger hooks (mobile-remote later), profiler timers
- [ ] Sandbox profiles: `game` (restricted os/io) vs `editor` (full)
- [ ] Docs: `docs/LUA_API.md` generated from bindings + examples
- [ ] Visual nodes: scope + prototype **after** V1 ships (Lua stays primary)

**Accept:** Lua-only 2D mini-game, zero C++ recompile.
**Tag:** `v1.0`

## What we deliberately defer

Networking, navmesh/pathfinding, skeletal animation retargeting, TileMap autotiling,
particles v2, consoles, visual scripting v1 — all tracked as `V1.1+` issues.

## Migration notes (prototype → V1)

| Prototype (M0–M6) | V1 |
|-------------------|----|
| `Audio` (miniaudio one-shots) | `AudioServer` (OpenAL buses + 3D) — prototype file removed |
| `Scene` static pools | Instance pools, multi-scene, `.arc` serialization |
| Inline demo shaders | `Material` + shader variants (GL + GLES) |
| `examples/0X-*` | Kept as regression; new `07/08/...` per phase |

**Tag:** `v1.0-p3`

- [ ] `Scripting` Lua 2D API: `transform2d`, `Input.axis/action`, `Audio.play`,
      `Physics.raycast2d`; `ready/process/physics_process`
- [ ] `examples/07-sprite2d`: player sprite + platforms + coin pickup + blip
- [ ] `projects/demo_2d/`: tiny platformer level in `.arc`

**Accept:** 60fps 2D collisions + Lua movement on Intel HD 620.
**Tag:** `v1.0-p1`
