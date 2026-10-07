# third_party
Vendored dependencies (built from source so Linux + Android use same code).

Planned:
- `lua/`      -> Lua 5.4 source (M9 scripting)
- `sol2/`     -> sol2 single header `sol.hpp` (M9 scripting)
- `glad/`     -> glad2 loader for Desktop GL + GLES (M2 onwards)
- `glm/`      -> math (M3 onwards)
- `stb/`      -> image loading (M6)
- `imgui/`    -> editor UI (M7)

For M0 we use system compiler only to prove CMake works.
Nothing to build here yet.
