# Reference Study — What ArcEngine Copies (and Avoids)

Sources on disk: `/home/arc/Documents/EngineReference/godot-src/godot-4.7.2-stable`
(~87k commits, MIT) and Unreal 5.8.3 zip (extracting). Studied via headers +
`scene/main/*`, `servers/*`, `drivers/*`, `main/main.cpp`. **Not copied** —
reimplemented clean-room in ArcEngine style (C++17, OpenGL, Lua).

## 1. Godot: COPY these ideas

- **Servers own state, Scene is thin.** `servers/audio|physics_2d|physics_3d|rendering`
  vs `scene/2d|3d`. Our `AudioServer/PhysicsServer/RenderingServer` mirror this.
  Proof: `servers/audio/audio_server.h` (buses, mix step), `servers/physics_2d/*`.
- **Audio buses.** `set_bus_count/add_bus/set_bus_volume_db` in `audio_server.h`.
  We adopt `Master → SFX → Music` + per-player `bus` property (like
  `AudioStreamPlayer2D/3D.bus`). OpenAL implements it via per-source gain.
- **2D + 3D audio parity.** `scene/2d/audio_stream_player_2d.h`
  (`attenuation`, `max_polyphony`, `pitch_scale`) and
  `scene/3d/audio_stream_player_3d.h` (`attenuation_model`, `unit_size`,
  `max_db`, `emission_angle`, `doppler_tracking`). Our `AudioPlayer2D/3D`
  copy these fields 1:1.
- **Fixed-step main loop.** `main/main.cpp` `Main::iteration()`:
  `iteration_prepare → physics_process (accumulator, max steps) →
  NavigationServer physics → process`. Our `App` loop copies this ordering.
- **Node notifications.** `node.h` `NOTIFICATION_READY=13`,
  `NOTIFICATION_PHYSICS_PROCESS=16`, `NOTIFICATION_PROCESS=17`.
  Our Lua lifecycle `ready/process/physics_process` maps directly.
- **Physics as modules.** `modules/` has `godot_physics_2d`, `godot_physics_3d`,
  `jolt_physics` — Jolt is vendored as a module, exactly our Box2D+Jolt plan.
- **GLES3 as compatibility renderer.** `drivers/gles3/` (`rasterizer_scene_gles3`,
  `rasterizer_canvas_gles3`) proves Canvas + Scene both run on GLES.
  Our GLES 3.2 backend follows the same split (canvas batcher + forward scene).

## 2. Godot: AVOID these mistakes

- **No OpenAL — custom mixers per OS.** `grep openal` hits only ICU unicode;
  real drivers are `drivers/alsa|pulseaudio|wasapi|coreaudio|xaudio2`.
  Lesson: Godot maintains ~6 audio backends. We use **one OpenAL path**
  (OpenAL-Soft covers Linux + Android) — far less code.
- **SCons + no-STL policy.** We keep CMake + STL (faster onboarding, NDK-ready).
- **2M-line scope.** We defer networking/navmesh/retargeting to V1.1+.

## 3. Unreal: COPY these ideas (from docs + zip layout)

- **Plugins/ + platform split.** `Engine/Plugins|Platforms|Programs` layout —
  our `third_party/` + `platform/android/` mirrors it at small scale.
- **UProject-style project file.** Our `project.arc` + `projects/demo_2d`
  mirrors `Default.uprojectdirs` thinking: project describes maps + assets.
- **Forward renderer for mobile.** Unreal's mobile forward path validates our
  forward-only V1 (no deferred) for GLES.

## 4. Unreal: AVOID these mistakes

- **C++ + Blueprints + macros (UCLASS/USTRUCT) complexity.** We do
  C++ servers + plain Lua (sol2, no codegen) — readable in one sitting.
- **256k-file, ~3GB tree.** ArcEngine stays vendored-minimal; every
  `third_party/*` must justify size (glad ~22k lines OK, Jolt justified, rest tiny).
- **Closed habits.** MIT + `docs/*.md` professional from day one (this file).

## 5. Resulting V1 deltas (applied to blueprint)

1. `AudioPlayer2D`: `attenuation`, `max_polyphony`, `pitch_scale`, `bus`.
   `AudioPlayer3D`: + `attenuation_model`, `unit_size`, `max_db`,
   `emission_angle`, `doppler` flag.
2. `App` loop order = Godot `Main::iteration` (prepare → physics steps →
   process → render), `NOTIFICATION_*` → Lua lifecycle.
3. GLES backend split = canvas (2D batch) + scene (3D forward), like `drivers/gles3`.
4. Physics = modules pattern (`Box2D 2D`, `Jolt 3D`), swappable behind server.
