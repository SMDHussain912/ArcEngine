#pragma once
#include "ArcEngine/Scene/Scene.h"

#include <string>

namespace Arc {

// .arc scene files (P0 Step 4, v2) — YAML via system yaml-cpp (0.9.0).
// V2 scope: entities + names + Transform + Mesh (builtin id + tint) +
// Camera/Light/GUI stubs. Unknown mesh ids fall back to triangle; a future
// material/asset pass will carry texture + gltf references.
//
//   scene:
//     format: arc-scene-2
//   entities:
//     - name: Player
//       transform:
//         position: [0, 0, 0]
//         rotation: [0, 0, 0]
//         scale:    [1, 1, 1]
//       mesh: { id: triangle, tint: [1, 0.2, 0.2] }
//       camera: { active: true, zoom: 1.0 }
//       light: { color: [1, 0.95, 0.85], intensity: 1.0, radius: 3.0 }
//       gui: { widget: button, size: [1.2, 0.4], text: "Play" }
//
// Entity ids are re-assigned on load (v1); stable ids/refs come when
// scripts need them. Save takes non-const Scene (Entities() iterates live).
// Loads arc-scene-1 too (mesh/camera/light/gui simply absent).
class SceneSerializer {
public:
    static bool Save(Scene& scene, const std::string& path);
    static bool Load(const std::string& path, Scene& outScene);
};

} // namespace Arc
