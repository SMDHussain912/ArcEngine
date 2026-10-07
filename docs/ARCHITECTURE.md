# ArcEngine Architecture — Servers + Scene (Godot-inspired)

How the engine is organized and why. Read `README.md` first, `V1_BLUEPRINT.md` for phases.

## 1. Big idea

Godot splits work into **backend Servers** that own state and a **SceneTree**
that users script. Unity gives users **Entities + Components**. ArcEngine does both:

- **Entities** are thin handles (`Scene*` + id) with components
  (`Transform2D/3D`, `Sprite2D`, `MeshInstance3D`, `Camera2D/3D`, `Body2D/3D`, `AudioPlayer2D/3D`, `LuaScript`).
- **Servers** own the real resources: GPU objects, physics worlds, audio voices.
  Game/Lua code never touches `gl*`, `al*`, `b2_*`, or Jolt directly.

## 2. Frame loop (fixed-step)

```
accumulator += delta
while (accumulator >= PHYSICS_DT):   # 1/60
    PhysicsServer.Step()
    ScriptServer.PhysicsProcess()
    accumulator -= DT
ScriptServer.Process(delta)           # variable render dt
RenderingServer.Draw()                # 2D canvas, then 3D forward
AudioServer.Update(listener poses)
```

2D and 3D share the loop. Physics never runs at render rate — determinism first.

## 3. Servers

### RenderingServer
- Owns: shaders/programs, VAOs/VBOs, textures, framebuffers, materials, lights.
- 2D: sorted canvas (layer + z), single batched quad draw where possible.
- 3D: forward pass (V1), per-material uniforms, shadow stub (V1.1).
- Backends: `OpenGLRenderer` (4.6 Core) now, `OpenGLESRenderer` (3.2) in P4.
  Shader sources carry both `#version 460 core` and `#version 320 es` variants.

### PhysicsServer
- 2D: Box2D v3 world (bodies, shapes, joints, raycast, layers/masks).
- 3D: Jolt world (rigid/character/static, raycast, layers).
- Fixed-step, interpolated render transforms (P2).

### AudioServer (OpenAL)
- One device + context. `Listener` (camera-follow), `Source` (2D/3D emitters),
  `Buffer` (decoded wav/ogg). Buses: Master → SFX → Music with gain.
- 2D: volume by distance on canvas plane. 3D: HRTF/doppler via OpenAL.
- Android: OpenAL-Soft over OpenSL ES / AAudio (P4).

### ScriptServer (Lua 5.4 + sol2)
- One `sol::state` per Scene (P5: per-world). Lifecycle mirrors Godot:
  `ready → process → physics_process → destroy`.
- Bindings expose Scene/Input/Audio/Physics/Assets. Hot-reload watches `.lua`
  mtime in editor Play mode. Sandbox: `game` profile strips `os.execute/io.open`.

### AssetManager
- Every asset has UUID (`assets/foo.png` + `assets/foo.png.import` with hash).
- Importers: texture (stb_image → GPU), audio (wav/ogg → OpenAL buffer),
  model (cgltf → Mesh + Materials), scene (YAML → Entity tree).
- Hot-reload: editor watches source hash, reimports, pushes to servers.

## 4. Threading (V1)

Single game thread + worker decode queue. No render thread yet (P3+):
`main: App/Input/Script/Physics/Render` · `workers: image/audio/model decode`.
Servers queue GPU uploads consumed before Draw.

## 5. Portability rules

- `Window` abstracts GLFW (Linux) vs EGL NativeActivity (Android).
- `Input` abstracts keyboard/mouse vs multi-touch.
- Shaders: no desktop-only GLSL in shared materials; precision qualifiers for GLES.
- Vendored `third_party/*` builds under NDK with no system package installs.
