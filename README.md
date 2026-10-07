<p align="center">
  <h1 align="center">ArcEngine</h1>
  <p align="center"><b>A production-grade 2D + 3D game engine for Linux & Mobile — inspired by Godot, built with C++, OpenGL & Lua.</b></p>
  <p align="center">
    <a href="https://github.com/SMDHussain912/ArcEngine"><img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License: MIT"></a>
    <img src="https://img.shields.io/badge/language-C++17-blue.svg" alt="C++17">
    <img src="https://img.shields.io/badge/graphics-OpenGL_4.6_GLES_3.2-orange.svg" alt="OpenGL">
    <img src="https://img.shields.io/badge/audio-OpenAL_3D-blueviolet.svg" alt="OpenAL">
    <img src="https://img.shields.io/badge/scripting-Lua_5.4_sol2-yellow.svg" alt="Lua">
    <img src="https://img.shields.io/badge/platforms-Linux_Android-lightgrey.svg" alt="Platforms">
  </p>
</p>

> **Status: V1 Blueprint.** Prototype M0-M6 done and tagged (v0.1.0 to v0.7.0-assets). Now hardening into a real engine — Godot-style architecture, Unity-style Entities, Lua gameplay.

## Table of Contents
- [Why ArcEngine?](#why-arcengine)
- [Features](#features)
- [Tech Stack](#tech-stack)
- [Architecture](#architecture-godot-inspired)
- [Quick Start](#quick-start)
- [Scripting with Lua](#scripting-with-lua)
- [Project Structure](#project-structure)
- [Roadmap / Phases](#roadmap--phases)
- [Contributing](#contributing)
- [License](#license)

## Why ArcEngine?

| Engine | Strength | Gap we fill |
|--------|----------|-------------|
| Unity | Best 2D+3D workflow | Closed source, heavy, weak Linux-first story |
| Godot | Open MIT, great 2D+3D servers | GDScript-centric; we want Lua-first + OpenGL/GLES + OpenAL lean mobile |
| **ArcEngine** | **Godot ideas + Unity Entities + Lua**, MIT, Linux-first to Android | Learnable core you can read end-to-end, shippable 2D/3D mobile games |

**Design goals:**
1. **2D + 3D in one Scene** — Sprite2D/Camera2D next to MeshInstance3D/Camera3D (Godot-style).
2. **Servers own state** — RenderingServer, PhysicsServer, AudioServer; Entities are thin handles.
3. **Lua is gameplay** — ready/process/physics_process lifecycle, hot-reload, sandboxed, sol2-bound.
4. **Mobile is a target, not a port** — every subsystem ships Desktop + GLES/Android paths.
5. **Small readable core** — no 2M-line maze.

## Features

**Working now (prototype, tagged):**
- [x] Window + loop (GLFW, VSync, FPS) — examples/01-window
- [x] OpenGL 4.6 Core via glad2 — examples/02-triangle
- [x] Math/Input/File — examples/03-core-utils
- [x] Renderer abstraction — examples/04-renderer
- [x] Scene + Entity + Transform + Mesh — examples/05-scene
- [x] Texture (stb_image) + Audio (miniaudio, replaced by OpenAL in V1) — examples/06-texture

**V1 production targets:**
- [ ] 2D: Camera2D, Sprite2D batcher, layers
- [ ] 3D: Camera3D, Material + lights, glTF models
- [ ] Physics: Box2D v3 (2D) + Jolt (3D), fixed-step, layers, raycasts
- [ ] Audio: OpenAL Listener/Source/Buffer, buses, 2D/3D positional, wav+ogg
- [ ] Scripting: Lua 5.4 + sol2 full API, hot-reload; visual nodes after V1
- [ ] Assets: AssetManager (UUIDs), importers, .arc scenes (YAML), hot-reload
- [ ] Editor: ImGui Hierarchy/Inspector/Viewport/Assets/Console + Play mode
- [ ] Mobile: NDK + EGL + GLES 3.2, touch, APK export

## Tech Stack

| Layer | Choice | Why |
|-------|--------|-----|
| Language | C++17 | sol2 + Jolt + Box2D compatible |
| Graphics | OpenGL 4.6 -> GLES 3.2 (glad2) | One shader family, proven on Intel HD 620 |
| Audio | OpenAL (OpenSL/AAudio on Android) | 3D positional standard; miniaudio was prototype-only |
| Physics 2D | Box2D v3 (vendored) | Standard 2D, Godot 4 option |
| Physics 3D | Jolt (vendored) | Fast modern rigid bodies, mobile-friendly |
| Scripting | Lua 5.4 + sol2 (vendored) | Tiny MIT, Android-clean |
| Math | glm | Shader-matching conventions |
| Images | stb_image | Single header PNG/JPG |
| Audio decode | stb_vorbis / minimp3 | wav + ogg without FMOD |
| Models | cgltf | Single-header glTF 2.0 |
| Scenes | yaml (ryml/yaml-cpp) | Human-readable .arc like Godot .tscn |
| Editor UI | Dear ImGui + docking | No Qt dependency |
| Windowing | GLFW (Linux) -> EGL (Android) | Hidden behind Window abstraction |
| Build | CMake 3.18+ | Same tree for Linux + NDK |

## Architecture (Godot-inspired)

```
              ArcEditor (ImGui): Hierarchy | Inspector | Viewport | Assets | Console
                                            | edits
ArcRuntime: App (Physics 60Hz -> Process -> Render) |
  Scene/Entity Tree (Node2D/Node3D, Sprite/Mesh, Camera, LuaScript)
    |--> RenderingServer (2D batcher, 3D forward, Materials, GLES path)
    |--> PhysicsServer (Box2D 2D + Jolt 3D, layers, raycasts)
    |--> AudioServer over OpenAL (Listener/Source/Buffer, buses)
    |--> ScriptServer (Lua 5.4 + sol2, hot-reload, sandbox)
    |--> AssetManager (UUID + importers + .arc scenes + hot-reload)
Platform: Linux (GLFW + GL 4.6) | Android (EGL + GLES 3.2 + touch)
```

**Rules:** game/Lua code uses Servers + Scene API only (never raw gl*/al*/b2_*); 2D+3D share one loop; every feature ships Desktop + GLES path or explicit TODO; assets are imported with UUID + hash cache.

**Godot to ArcEngine map:** Node/Node2D/Node3D -> Entity + Transform; Sprite2D/Camera2D -> same names (batched); Camera3D/MeshInstance3D/Material/DirectionalLight -> same; CharacterBody/RigidBody/StaticBody/Area -> Box2D/Jolt bodies; AudioStreamPlayer2D/3D + buses -> AudioPlayer2D/3D + AudioBus; GDScript ready/process -> Lua ready/process/physics_process.

## Quick Start

Prereqs (Linux): g++ 11+, cmake 3.18+, glfw3, libGL, openal, lua5.4 dev packages.

```bash
git clone https://github.com/SMDHussain912/ArcEngine.git
cd ArcEngine
git checkout dev && git pull origin dev
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
cd ..   # run from ROOT so assets/ resolves
./build/examples/05-scene/05-scene
./build/examples/06-texture/06-texture
```

Controls: WASD/arrows move, Space action, ESC quit.

## Scripting with Lua

```lua
-- assets/scripts/player.lua
local player = {}
function player:ready()
  Log.info("Player ready: " .. self:name())
  self.speed = 220.0
end
function player:process(dt)
  local x = Input.axis("move_left", "move_right")
  local t = self.transform2d
  t.position.x = t.position.x + x * self.speed * dt
  if Input.pressed("jump") then Audio.play("assets/audio/blip.wav") end
end
return player
```

Lifecycle ready/process/physics_process/destroy (Godot-like). Hot-reload in editor Play mode. Sandboxed in games, full access in editor. Visual nodes after V1.

## Project Structure

```
engine/            libArcEngine.a (Core, Scene, Rendering, Physics, Audio/OpenAL, Scripting/Lua, Assets)
editor/            ArcEditor (ImGui panels)
runtime/           ArcRuntime (loads project.arc, runs App loop)
examples/          01-window .. 06-texture (+ V1: 07-sprite2d, 08-physics2d ...)
assets/            shaders, textures, audio, scripts, scenes/*.arc
third_party/       glad, stb, sol2, lua, box2d, jolt, imgui, cgltf, yaml
tools/             make_test_png.py, make_test_wav.py
tests/             unit + headless scene tests
docs/              V1_BLUEPRINT.md, ARCHITECTURE.md, ROADMAP.md (M0-M6 history)
projects/          sample games (V1): demo_2d/, demo_3d/
```

## Roadmap / Phases

Full detail in docs/V1_BLUEPRINT.md. History in docs/ROADMAP.md.

| Phase | Goal | Deliverables | Done when |
|-------|------|--------------|-----------|
| P0 Harden | Production foundation | App fixed-step loop, AssetManager + .arc, OpenAL replaces miniaudio, CI | Empty .arc boots; OpenAL blip plays |
| P1 2D | Shippable 2D | Camera2D, Sprite2D batcher, PhysicsServer2D (Box2D), Lua 2D API, demo | 2D collisions + Lua movement at 60fps |
| P2 3D | Shippable 3D | Camera3D, Material + lights, glTF Model, PhysicsServer3D (Jolt), Lua 3D | Lit 3D level + rigid body + orbit cam |
| P3 Editor | Build visually | ImGui panels, Play mode, gizmos | Edit + play 2D without C++ |
| P4 Mobile | Android export | GLES parity, touch, NDK, APK | Demos run on phone |
| P5 Scripting+ | Lua complete | Full API, debugger hooks, sandbox, hot-reload | Lua-only mini-game, no recompile |

Non-goals V1: networking, navmesh, skeletal retargeting, visual scripting, consoles.

## Contributing

```bash
git checkout dev && git pull origin dev
git checkout -b feature/<phase>-<what>
git push origin feature/<phase>-<what>   # PR to dev
```

main = releases, dev = integration, feature/* = work, tags v1.0-p0, v1.0-p1... No raw gl*/al*/b2_* outside Servers. Update blueprint checkboxes per PR.

## License

MIT — see LICENSE. Your games are yours, no royalties.
