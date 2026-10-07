# ArcEngine Roadmap — Linux first, Mobile second

Stack: `C++17 + OpenGL 4.6 -> GLES 3.2 + Lua 5.4 + sol2 + GLFW (Linux) -> SDL/EGL (Android)`

## M0 Foundation [DONE — v0.1.0]
- CMake 3.18+, `ArcEngine` static lib + `ArcRuntime` exe
- Version API, README, MIT, .gitignore
- Verified: `g++ 16.2.1 + cmake 4.4.3` builds + runs

## M1 Window + Loop [DONE]
- `engine/core/Window` RAII wrapper over GLFW (VSync, resize, ESC-close, FPS title)
- `engine/core/Log` colored console logger, `engine/core/Time` delta/FPS clock
- Main loop with `glClear` pulse (no shaders yet — M2), `examples/01-window`
- Verified on Intel HD 620: both binaries open windows, run loop, exit 124 on timeout (expected)

## M2 Triangle [DONE]
- Vendored `glad2` (Desktop GL 4.6 Core) in `third_party/glad/` + `glad` static lib
- `engine/renderer/GraphicsContext` (gladLoadGL via GLFW), `Shader` (compile/link/file), `Buffer` (VBO/IBO/VAO)
- `Window` now requests 4.6 Core explicitly + `GLFW_INCLUDE_NONE` everywhere (glad owns GL headers)
- `assets/shaders/triangle.vert/frag` (#version 460 core, RGB triangle), `examples/02-triangle`
- M1 apps ported to glad init; verified on Intel HD 620 Mesa 26.2.3: `OpenGL loaded: 4.6 (Core Profile)`

## M3 Core utils [DONE]
- `Math.h`: Vec2/3/4, Mat3/4, Quat over glm 1.0.3 + Translate/Rotate/Scale/Perspective/Ortho/LookAt/Lerp/Clamp
- `Input`: Key/MouseButton polling, IsKeyDown/Pressed, MousePos, EndFrame edge detection
- `File`: Exists/ReadText/ReadBinary/WriteText
- `Shader::SetVec2` added; `examples/03-core-utils` = movable triangle (WASD, Space invert, click log)

## M4 Renderer abstraction [DONE]
- `Renderer` interface (BeginFrame/EndFrame/DrawArrays/DrawIndexed/SetViewport) + `CreateRenderer()` factory
- `OpenGLRenderer` (4.6 Core, depth on) — M8 adds `OpenGLESRenderer` behind same factory
- `RenderCommand` (SetClearColor/Clear/SetViewport) — game code never calls raw gl* for frame control
- Fixed `Input::EndFrame` valid key ranges (32-96, 256-348); was spamming `Invalid key 349`
- `examples/04-renderer`: triangle via abstraction only, WASD moves

## M5 Scene / Entities
- `Scene, Entity, TransformComponent`

## M6 Assets + Audio
- `stb_image` textures, shader reload, `miniaudio`

## M7 Editor
- ImGui: Hierarchy, Inspector, Viewport

## M8 Android
- NDK + EGL + GLES, touch input, APK

## M9 Lua scripting (agreed: Lua 5.4 + sol2, C++17)
- Vendor `third_party/lua` + `third_party/sol2/single/sol.hpp`
- `engine/script/ScriptSystem` owning `sol::state`
- Lifecycle: `Awake/Start/Update(dt)/OnDestroy`, hot-reload
- Sandbox `os/io` for shipped games

## Git
- `main` stable, `dev` integration, `feature/*` work
- Tags: `v0.1.0`, `v0.2.0-window`, ...
