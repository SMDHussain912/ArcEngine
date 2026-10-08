// P0 headless test: Project load + save round-trip (no window/GL needed).
// Run from the REPO ROOT: ./build/tests/test_project
#include "ArcEngine/Core/Project.h"

#include <cstdio>
#include <cstdlib>
#include <string>

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main() {
    // 1. Load the checked-in sample.
    auto p = Arc::Project::Load("projects/empty/project.arc");
    CHECK(p.has_value(), "sample project.arc must load");
    CHECK(p->Name == "ArcEngine Empty Project", "name parsed");
    CHECK(p->Version == "0.1.0", "version parsed");
    CHECK(p->Width == 1280 && p->Height == 720, "window size parsed");
    CHECK(p->VSync == true, "vsync parsed");
    CHECK(p->PhysicsHz == 60.0f, "physics_hz parsed");
    CHECK(p->WindowTitle == "ArcEngine — Empty Project", "title parsed");
    CHECK(p->MainScene.empty(), "empty main_scene allowed");

    // 2. ToAppConfig mapping.
    Arc::AppConfig cfg = p->ToAppConfig();
    CHECK(cfg.Title == "ArcEngine — Empty Project", "cfg title from window.title");
    CHECK(cfg.Width == 1280 && cfg.PhysicsHz == 60.0f, "cfg window/physics mapped");

    // 3. Save -> Load round-trip.
    const std::string tmp = "/tmp/arc_test_roundtrip.arc";
    CHECK(p->Save(tmp), "save to tmp");
    auto p2 = Arc::Project::Load(tmp);
    CHECK(p2.has_value(), "reload saved file");
    CHECK(p2->Name == p->Name && p2->Version == p->Version, "round-trip name/version");
    CHECK(p2->Width == p->Width && p2->Height == p->Height, "round-trip size");
    CHECK(p2->PhysicsHz == p->PhysicsHz && p2->VSync == p->VSync, "round-trip physics/vsync");
    CHECK(p2->WindowTitle == p->WindowTitle, "round-trip title");
    std::remove(tmp.c_str());

    // 4. Missing file -> nullopt (no crash).
    auto missing = Arc::Project::Load("/nonexistent/nope.arc");
    CHECK(!missing.has_value(), "missing file returns nullopt");

    std::printf("test_project: ALL OK\n");
    return 0;
}
