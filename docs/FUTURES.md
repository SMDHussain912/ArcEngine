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
- [x] Step 4 Collision events: DrainEvents after every sub-step (canonical pairing, exactly-once drain, stale-id nulling), PollCollisionEvents + SensorTouched, RemoveBody/Destroy purge — `tests/test_physics_events.cpp`
- [x] Step 5 CircleCollider2D: `Radius` on BoxCollider2DComponent (Radius > 0 → b2Circle, else box), .arc round-trip, viewport circle outline, Inspector "Radius (0 = box)" — verified in `tests/test_physics_serialize.cpp` (+ linkage fix: box2d objects now linked into ArcEngine)
- [x] Step 6 CapsuleCollider2D: `Height` on BoxCollider2DComponent (Height > 0 → b2Capsule, vertical segment + Radius caps; then Radius > 0 → circle; else box), .arc round-trip, viewport capsule outline, Inspector field — rest-height tests prove the real shape (`tests/test_physics.cpp` 4b), round-trip in `tests/test_physics_serialize.cpp`
- [x] Step 7 PolygonCollider2D: `Vertices[8]` + `VertexCount` on BoxCollider2DComponent (VertexCount ≥ 3 → `b2ComputeHull` + `b2MakePolygon`; degenerate hull → box fallback with error log), .arc `polygon` round-trip, viewport outline, Inspector vertex editor — diamond rest-height + fallback tests (`tests/test_physics.cpp` 4c), round-trip in `tests/test_physics_serialize.cpp`
- [ ] Step 8 ChainCollider2D (static ground outlines, one-sided segments/loop) + EdgeCollider2D lite, .arc
- [ ] Step 9 Compound bodies: N shapes per body (box + sensor-feet pattern), per-shape material + per-shape sensor flag, AddShape/RemoveShape API, test
- [ ] Step 10 Joints: RevoluteJoint2D + DistanceJoint2D (anchorA/anchorB, limits, motor lite), PrismaticJoint2D stretch, WeldJoint2D lite, .arc, test
- [ ] Step 11 CharacterBody2D: kinematic move-and-slide helper (velocity, snap, floor normal, coyote-time lite), test on slopes/steps
- [ ] Step 12 Area2D triggers: sensor-only body + OnAreaEnter/Exit via events (coin pickup path), test
- [ ] Step 13 Debug draw: shape outlines (all types) + contact points + AABBs in viewport (toggle)
- [ ] Step 14 Named layers v2: 32 named physics/render layers ("Player","Ground") replacing raw hex Category/Mask, editor dropdown, .arc by name
- [ ] P1 demo: `projects/demo_2d/` platformer .arc (player + platforms + coins + blip) + `examples/09-*`; accept: 60fps collisions on Intel HD 620

### GUI system v1 — runtime UI (today: 3 widget kinds, editor-viewport-only, ZERO runtime draw/input — buttons you can't press)
> Problems to fix: (1) no runtime renderer — GuiComponent invisible in ArcRuntime; (2) no input — no hover/press/click/focus; (3) only Button/Label/Panel — missing CheckBox/Slider/TextField/etc; (4) fixed world pos — breaks on resize, no anchors/layout; (5) no text rendering — stb_truetype vendored but unused; (6) per-widget colors — no shared theme; (7) no callbacks — gameplay can't react.
- [ ] Step G1 GuiRenderer: screen-space overlay pass in runtime (camera-independent, resolution-aware), draws Panel (rounded rect) + Label (font atlas) + Button (normal/hover/pressed states) — `engine/*/Gui/GuiRenderer.*`, `examples/10-gui`
- [ ] Step G2 Input + focus: mouse hover/press/release hit-testing (top-most first), keyboard focus chain (Tab), click-consume so game doesn't double-fire, test headless with synthetic input
- [ ] Step G3 Widgets pack 1: CheckBox (toggled bool), Slider (float min/max/step + drag), ProgressBar (0..1 fill), Image/Icon (texture + tint + UV)
- [ ] Step G4 Widgets pack 2: TextField (focus, caret, typing, backspace, max-length), Dropdown (option list popup), ScrollView (clip + scrollbar), Tooltip (hover delay text)
- [ ] Step G5 Anchors + layout: anchor presets (top-left/center/full-rect...), HBox/VBox containers (padding/spacing), auto-resize on window change, min-size clamp
- [ ] Step G6 Text engine: stb_truetype font atlas (ASCII + Latin-1), size/align/word-wrap/ellipsis, Label auto-size, .arc font path
- [ ] Step G7 Theme/skins: one GuiTheme struct (colors, corner radius, padding, font size) shared by all widgets, editor theme editor lite, .arc theme block
- [ ] Step G8 Callbacks: on_pressed / on_toggled / on_value_changed (C++ std::function now, Lua bindings in P5), signal connect/disconnect API, test
- [ ] Step G9 Editor: Inspector UI for every widget (all fields), widget creation menu, viewport widget outlines + click-to-select, .arc round-trip for all widgets — extend `tests/test_scene.cpp` or new `tests/test_gui.cpp`
- [ ] GUI demo: `examples/10-gui` main-menu (title + Play/Quit buttons + volume slider + name field) driving real state; accept: mouse-only navigation, resize-safe

### Gameplay systems (engine is basic without these — nothing animates/moves/scrolls today)
> Animation + particles live in their own section below (A1–A10) — bigger than first scoped.
- [ ] TileMap: grid + tileset atlas + batched chunk draw + autotile-lite (bitmask 16-tile), collision import from tile flags, `examples/13-tilemap`
- [ ] Camera2D: follow target + deadzone + lookahead, trauma shake, bounds limits, zoom smoothing, editor preview
- [ ] ParallaxBackground: layered scroll factors + infinite wrap (modulo), editor layer list
- [ ] SaveGame system: save slots (JSON-ish over .arc helpers), auto-save lite, version field + migration note, test
- [ ] Object pooling: generic Pool<T> (bullets/enemies/particles), prewarm + grow cap, test

### Engine-quality basics
- [ ] Prefabs: save entity subtree → stamp copies (enemies/bullets/coins), override lite, .arc prefab files, test
- [ ] Tags + Groups: string tags per entity, FindWithTag/FindInGroup, group broadcast call, test
- [ ] Undo for everything: extend undo/redo beyond transforms (component add/remove/edit, physics fields, GUI fields)
- [ ] Profiler overlay: frame ms + physics ms + draw calls + binds + body count (surface existing counters), toggle F3, log-to-file lite
- [ ] Asset browser panel: real file view + import status (today stub), double-click open, drag-to-scene lite
- [ ] Headless CI: `cmake --build` + ALL tests in GitHub Actions (linux-runner, no GL needed for headless suites)

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

## P2 — Shippable 3D (expanded: "stylized-mobile 3D" lane — clean look that runs on a phone, GLES-safe from day one)
> Our 3D today: colored 2D primitives only (triangle/quad/circle/ring/plane/cross/arrow), no 3D camera/lights/textures/models/physics. Stubs reserved in MeshId (`gltf:<path>`, `prim:cube|sphere`).
> Research: Godot 4 (Forward+/Mobile/Compatibility — we build ONE scalable forward renderer), glTF 2.0 ISO ("JPEG of 3D", load via single-header cgltf, PBR metallic-roughness core), UE5 ideas scoped down (Nanite→LOD chain, Lumen→baked lightmap, World Partition→chunk streaming), Jolt (rigid + virtual character controller + vehicles).

### 3D Rendering core
- [ ] Step 3D-1 Camera3D: perspective + orbit helper (yaw/pitch/dist, damping), active stack with Camera2D, frustum culling lite, editor orbit gizmo
- [ ] Step 3D-2 Material: albedo/roughness/metallic (+emissive), dir+point+spot lights (count-capped forward pass), real shading hookup for LightComponent, GLES precision-safe shaders — `engine/*/Rendering/Material.*`
- [ ] Step 3D-3 Primitives 3D: cube/sphere/cylinder/cone/capsule/torus (indexed, normals+UVs), wireframe toggle, viewport primitive menu
- [ ] Step 3D-4 Forward renderer: depth-tested opaque pass + alpha pass, per-material uniforms, fog (exp2 + height lite), sky gradient + sun, gamma-correct output
- [ ] Step 3D-5 Baked lighting lite: lightmap UV bake tool (simple unwrapper), ambient probe (SH-lite 9 coeffs), fake AO (cavity from normals), emissive bloom-lite (threshold + blur mip) — 80% of AAA look, 0% realtime GI cost

### 3D Models + animation
- [ ] Step 3D-6 glTF 2.0 import via cgltf (vendor third_party/cgltf): nodes + meshes + PBR materials + textures + skins, `gltf:<path>` MeshId live, importer warnings panel, test with sample models — `examples/14-model3d`
- [ ] Step 3D-7 Skeletal animation: skinning shader path, clip player (play/blend/crossfade), bone debug draw, AnimationPlayer 3D tracks
- [ ] Step 3D-8 Mesh LOD chain (Nanite-lite): 3 auto levels per mesh, distance-switch with hysteresis, editor LOD preview, perf test
- [ ] Step 3D-9 Static Batching + instancing: merge static meshes per material, GPU instancing for repeats (trees/rocks), draw-call counter in profiler

### 3D Physics (Jolt — vendor third_party/jolt)
- [ ] Step 3D-10 PhysicsServer3D facade (mirror PhysicsWorld PIMPL pattern): RigidBody3D, StaticBody3D, box/sphere/capsule/convex-hull shapes, layers, raycast, fixed 1/60 — `tests/test_physics3d.cpp`
- [ ] Step 3D-11 Character controller via Jolt virtual character (don't hand-roll): walk/sprint/jump, slopes/steps, moving platforms lite, test course
- [ ] Step 3D-12 Vehicle lite (Jolt vehicle system): arcade car (steer/throttle/brake, drift factor), `examples/15-vehicle`
- [ ] Step 3D-13 Ragdoll lite: jointed bodies from skeleton, blend animated→ragdoll on death, test

### 3D Gameplay + world
- [ ] Step 3D-14 ThirdPersonController: follow cam + collision (raycast pull-in), input-relative movement, jump/ coyote, test scene
- [ ] Step 3D-15 Level streaming (World-Partition-lite): chunked collections + load radius, async decode queue, loading screen hook, `projects/demo_3d/` playground .arc
- [ ] Step 3D-16 see A10 (3D particles + trails merged into Animation section)
- [ ] Step 3D-17 Sky + weather lite: time-of-day curve (sun angle/color, CanvasModulate-3D), rain/snow particle presets, puddle-roughness note
- [ ] Lua 3D API: transform3d, camera helpers, spawn_model; accept: lit 3D level + rigid bodies + orbit cam, GLES-compatible shaders
- [ ] P2 demo: `examples/14-model3d` lit glTF room + falling crates + orbit camera — tag v1.0-p2

### 3D World-building (we have ZERO — researched LearnOpenGL terrain + O3DE gems)
- [ ] Step 3D-18 Terrain: heightmap import → CPU mesh (no GPU tessellation — GLES can't), 4-texture splatmap blend (grass/rock/sand/snow by height+slope), hole flag, editor height brush lite — `examples/16-terrain`
- [ ] Step 3D-19 Water lite: animated normal-scroll plane + fresnel + sky tint (no mirrored scene — too costly on phones), shore foam band, swim-blocker volume. Lakes, not oceans
- [ ] Step 3D-20 Sky v2 + clouds: gradient + sun disc + noise-scrolled cloud billboard layer, day-night curve drives sun/fog/ambient together
- [ ] Step 3D-21 Vegetation lite: instanced grass tufts (crossed quads + wind sway shader) + trees (2 LODs + billboard far), editor scatter brush with density mask
- [ ] Step 3D-22 White-box tools: sculpt/place geometric volumes in-editor to sketch levels fast (O3DE White Box idea), one-click convert to static meshes

### 3D Look-dev (beyond fog + bloom-lite)
- [ ] Step 3D-23 Post FX stack: ONE fullscreen pass — vignette + color grade (lift/gamma/gain LUT-lite) + FXAA + camera-motion-blur-lite, per-effect toggles, GLES-safe
- [ ] Step 3D-24 Decals: projected quads pool (bullet holes, footprints, scorch), fade-out + cap, editor decal preview
- [ ] Step 3D-25 Shadows v1: single directional shadow map (2048, PCF-lite) + blob-shadow fallback for phones

### 3D Feel + systems (invisible plumbing every 3D game needs)
- [ ] Step 3D-26 Camera effects: FOV kick (sprint), trauma shake (port from 2D), underwater tint zone, landing dip
- [ ] Step 3D-27 3D audio hooks: positional OpenAL emitters + reverb zones (cave vs field), footstep material switch (grass/stone/wood)
- [ ] Step 3D-28 Interaction system: raycast interact (press E: pickup/door/NPC), highlight outline, hold-to-use progress bar, test scene

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

### Animation system (was 3 thin lines — now the full Godot/Spine-inspired stack)
> Today: "frame grid" + "Tween lite" + "clip player" — NO state machine, NO tool, NO Spine, NO IK.
> Research: Unity Shuriken (modular emission/shape/curves/trails/sub-emitters), Spine (bones+attachments+IK+skins+blend, tiny data, spine-c runtime), Godot AnimationTree (states+transitions+Blend2D+crossfade).
- [ ] Step A1 SpriteSheet v1: frame grid (cols/rows/fps/loop/pingpong), flipbook player (play/stop/speed), frame events ("footstep on frame 12"), .arc, test — `examples/11-animation`
- [ ] Step A2 Tween engine: pos/scale/alpha/rotation easings (linear/quad/cubic/sine in-out), chaining + callbacks + kill, test
- [ ] Step A3 AnimationPlayer (any-property tracks): keyframes on transform/modulate/light/UI values, dopesheet editor panel (timeline + keys + tangents), .arc clips — THE animator foundation
- [ ] Step A4 AnimationTree (YOU CAUGHT THIS — was missing): state machine (idle/run/jump nodes, transitions with conditions, crossfade time), Blend2D directional + 1D blend, additive layers (walk+shoot), editor state-graph panel, test
- [ ] Step A5 IK 2-bone (2D + 3D): foot planting on ground, look-at/aim procedural layer, pole targets, editor IK preview
- [ ] Step A6 Root motion: movement extracted from animation delta (authoritative for fighters), loop-match + snap-back guard, test
- [ ] Step A7 Spine 2D (vendor spine-c): bones+slots+attachments, skins/outfit swap, IK + FFD-lite, animation mix/crossfade, bounding-box → Box2D sensor import, `examples/17-spine`
- [ ] Step A8 Retargeting (promote from V1.1): same-skeleton clip reuse across characters, bone-name map + scale adjust, test on 2 rigs
- [ ] Step A9 2D particles v2 (Shuriken-lite): Emission (rate/burst) + Shape (cone/sphere/box/circle) + Velocity/Color/Size-over-lifetime CURVES + Noise (curl-lite) + Trails + Sub-emitters (explosion→smoke) + Collision (bounce planes), curve editor panel, ONE batched draw, `examples/12-particles`
- [ ] Step A10 3D particles + trails: billboarded GPU points, ribbon trails (swords/wheels), floor kill plane, collision-lite — merge with 3D-16

### Game feel + game-loop plumbing (invisible, every shipped game needs)
- [ ] Time system: global time-scale (slow-mo/hit-stop/pause), per-group scale (UI ignores pause), fixed-step accumulator guard already have — add interpolation flag, test
- [ ] Scene manager: change_scene + additive load + unload, loading screen (progress + tips), transitions (fade/wipe/slide), scene stack for pause menus, test
- [ ] Juice kit (OUR LANE): hit-stop frames, screen shake (trauma-based, 2D+3D), kill-flash + hit-flash material pulse, damage numbers + floating text pool, squash-and-stretch helper, landing dust hook — `examples/18-juice`
- [ ] Dialogue + quests: dialogue tree nodes (line/choice/condition/jump) + portraits + typewriter + skip, quest log (objective/check/reward), .arc dialogue files, `examples/19-dialogue`
- [ ] Music system: crossfade player, layered stems (combat drums fade in), shuffle/mood playlists, duck-on-dialogue, test
- [ ] Input mapping v2: named actions (jump→Space+PadA+Touch), runtime rebinding UI widget, gamepad (SDL-lite: sticks/triggers/rumble), touch buttons overlay for mobile
- [ ] Replay system: record inputs per fixed-step → verify determinism (we're already fixed-step single-threaded — free win), replay file + playback, desync detect, netcode foundation note
- [ ] Debug console: in-game command line (`give_coins 99`, `godmode`, `tp x y`), command registry API, history + autocomplete lite, F8 toggle
- [ ] Capture + share: screenshot PNG key, GIF-lite recorder (fixed 320px, 10s cap) for sharing moments

## X — Export + Publish ("ArcBuilder": press Build, get a game — researched Godot presets + itch/Steam/Play checklists)
> Today: ONE Gradle line in P4. No presets, no packer, no signing, no store path. A game you can't ship is a toy.
> Research: Godot (export presets + templates + per-platform options + keystore signing + runnable dry-run), itch.io (one zip per platform + demo flag + press keys + page art 630x500), Steam/Play (achievements/cloud, Asset Delivery).
- [ ] Step X1 Export presets: per-project `export.arc` (name, id, version, icon, orientation, presets list), editor Export dialog (preset table + options + dry-run validate), `tools/arc_export` CLI (same pipeline headless for CI)
- [ ] Step X2 Asset packer: collect project.arc deps → content-addressed `.arcpak` (lz4-lite), UUID sidecars honored, hash-skip unchanged, mount priority (base → patch → DLC), test round-trip
- [ ] Step X3 Linux builder: folder + AppImage + .tar.gz, .desktop + icon, rpath-fixed libs, `chmod +x` smoke-run test, itch-zip layout (one zip, README, demo flag)
- [ ] Step X4 Android builder (APK first, AAB second): android/ template + Gradle wrapper pin, debug key auto-sign, release keystore wizard (generate + store password), icons/splash densities, permissions picker (vibrate/internet), versionCode bump, install-to-device button (`adb install -r`), Play Asset Delivery note for DLC
- [ ] Step X5 Web lite (stretch, after P4): Emscripten build (GLES→WebGL2, single .html + .wasm + .pak), itch HTML5 embed sizing, loading bar, audio-unlock-on-tap note
- [ ] Step X6 Store checklist panel: per-target status (icons? signed? version bumped? store art 630x500?), press-key generator, Steam AppID + achievements/cloud stub file, Play listing stub (feature graphic text), pre-flight warnings before Build
- [ ] Step X7 One-click publish: Build → Zip → Upload (itch.io Butler-compatible layout + API-key slot), release-notes prompt, channel (stable/beta/demo), rollback note (keep last 3 builds)

## ★ — Own ideas (nobody does these — OUR signature)
> The lane: "the engine that makes your game FEEL alive in a weekend." Every ★ item is demoable in <60s.
- [ ] ★ Juice Timeline: record gameplay → scrub slow-mo replay with auto juice-markers (hits, landings, pickups) → export GIF/video. Replay (planned) + capture (planned) FUSED into one tool. No engine has this.
- [ ] ★ Feel presets: one-click "game feel packs" (Juicy Platformer / Punchy Shooter / Cozy RPG): shake curves + hit-stop ms + flash colors + sound pitch range + particle bursts, all tuned, all overridable. Godot/GDevelop make you hand-tune; we ship taste.
- [ ] ★ Ghost multiplayer lite: record your run → race your ghost (platformers) / review enemy ghost (fighters). Replay gives it free; deterministic core makes ghosts EXACT.
- [ ] ★ Living levels: ambient-life emitter (birds/insects/leaves drift through scenes on spline paths, zero gameplay cost) + day-cycle director (light/desat/audio shift by clock). Dead scenes feel empty; ours breathe by default.
- [ ] ★ Director mode: in-editor "auto-playtest bot" (random-but-seeded inputs) that plays your level, reports stuck spots + FPS dips + unreachable coins, drops pins on the map. Playtesting without players.
- [ ] ★ Remix builds: player-facing variant system — ship ONE game, players remix (speed x2, low-grav, mirror world) via seed string; engine enforces determinism so remixes are shareable + leaderboard-fair.
- [ ] ★ Instant phone preview: editor "Preview" button → LAN server → phone browser/ArcPreview app streams the running game (GDevelop QR idea, ours over plain HTTP + input echo). No cable, no APK, 5 seconds.

## V1.1+ (deliberately deferred — do NOT build in V1)
- Networking, navmesh/pathfinding, TileMap autotiling, consoles, visual scripting v1.
  (Promoted OUT: skeletal retargeting → A8, particles v2 → A9.)

## Next up (agreed order)
1. Finish Step 4 collision events (drain wiring + test + push).
2. Steps 5–8 more collider shapes (circle → capsule → polygon → chain).
3. GUI v1 starts: G1 renderer + G2 input (buttons you can actually press).
4. SpriteSheet animation + Tween (things start moving).
5. Camera2D + Particles v1 + TileMap (levels with juice).
