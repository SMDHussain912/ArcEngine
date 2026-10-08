#pragma once
#include "ArcEngine/Scene/Scene.h"

#include <string>

namespace Arc {

// .arc scene files (P0 Step 4) — YAML via system yaml-cpp (0.9.0).
// V1 scope: entities + names + TransformComponent. Mesh/Texture/LuaScript
// serialization arrives with those subsystems (their data references assets).
//
//   scene:
//     format: arc-scene-1
//   entities:
//     - name: Player
//       transform:
//         position: [0, 0, 0]
//         rotation: [0, 0, 0]
//         scale:    [1, 1, 1]
//
// Entity ids are re-assigned on load (v1); stable ids/refs come when
// scripts need them. Save takes non-const Scene (Entities() iterates live).
class SceneSerializer {
public:
    static bool Save(Scene& scene, const std::string& path);
    static bool Load(const std::string& path, Scene& outScene);
};

} // namespace Arc
