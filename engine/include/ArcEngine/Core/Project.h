#pragma once
#include "ArcEngine/Core/App.h"

#include <cstdint>
#include <optional>
#include <string>

namespace Arc {

// Project descriptor (P0 Step 2): a small text file `project.arc` describing
// the game — name, version, main scene, window/physics defaults.
//
//   # comments are full lines
//   [project]
//   name = My Game
//   version = 0.1.0
//   main_scene = assets/scenes/main.arc
//   [window]
//   width = 1280
//   height = 720
//   vsync = true
//   physics_hz = 60
//
// Parser is intentionally tiny (hand-rolled key=value). Rich `.arc` SCENES
// with entity trees get a proper YAML lib later — we'll pick then.
struct Project {
    std::string Name = "Untitled";
    std::string Version = "0.0.1";
    std::string MainScene;   // may be empty for boot-only projects

    std::string WindowTitle; // empty -> Name
    uint32_t Width = 1280;
    uint32_t Height = 720;
    bool VSync = true;
    float PhysicsHz = 60.0f;

    std::string FilePath;    // where it was loaded from (asset-root base later)

    // Returns nullopt + logs reason on failure (missing/malformed file).
    static std::optional<Project> Load(const std::string& path);
    bool Save(const std::string& path) const;

    AppConfig ToAppConfig() const;
};

} // namespace Arc
