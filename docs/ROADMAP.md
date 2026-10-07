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

## M2 Triangle [NEXT]
- `glad2` loader, `Shader`, `Buffer`, `OpenGLRenderer`
- `examples/02-triangle`, shaders in `assets/shaders/`
- Request OpenGL 4.6 Core explicitly (M1 used compat defaults)

## M3 Core utils
- Vendored `glm` math, `Input` polling, `File` helpers

## M4 Renderer abstraction (key for mobile)
- `Renderer` interface -> `OpenGLRenderer` (4.6) / `OpenGLESRenderer` (3.2)

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
