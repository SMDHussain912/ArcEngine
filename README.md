# ArcEngine

A lightweight Unity-like game engine for **Linux first, Mobile second**.

Core: `C++17 + OpenGL 4.6 (Desktop) -> OpenGL ES 3.2 (Mobile)` | Scripting: `Lua 5.4 + sol2`

## Vision
Build a tiny, hackable, MIT-licensed engine you can learn from scratch:
`Window -> Renderer -> Scene/Entities -> Assets -> Editor (ImGui) -> Android -> Lua scripting`

## Status
- [x] M0: Foundation (CMake + git + GitHub)
- [x] M1: Window + Loop (GLFW + Log + Time)
- [x] M2: Triangle (OpenGL 4.6 Core + glad + Shader + VAO/VBO)
- [x] M3: Core utils (Math/glm + Input + File)
- [ ] M4: Renderer abstraction (GL / GLES)
- [ ] M5: Scene / Entity
- [ ] M6: Assets + Audio
- [ ] M7: Editor
- [ ] M8: Android
- [ ] M9: Lua scripting

## Quick Start (Linux)

```bash
git clone https://github.com/SMDHussain912/ArcEngine.git
cd ArcEngine
mkdir build && cd build
cmake ..
cmake --build .
./runtime/ArcRuntime          # full runtime (1280x720, FPS in title, ESC to close)
./examples/01-window/01-window # minimal example (960x540)
```

Requirements: `g++ (>=11), cmake (>=3.18), glfw3, libGL` on dev machine.
Vendored builds (no system deps) coming via `third_party/`.

## Workflow
```bash
git pull origin dev
git checkout -b feature/xxx
# ... code ...
git push origin feature/xxx
```
`main` = stable, `dev` = integration.

## License
MIT — see [LICENSE](LICENSE).
