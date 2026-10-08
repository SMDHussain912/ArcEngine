// P0 headless test: .arc scene Save->Load round-trip + instance-pool isolation.
// Run from the REPO ROOT: ./build/tests/test_scene
#include "ArcEngine/Scene/Scene.h"
#include "ArcEngine/Scene/SceneSerializer.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            return 1;                                                        \
        }                                                                    \
    } while (0)

namespace fs = std::filesystem;

static bool Near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

static bool VecNear(const Arc::Vec3& a, const Arc::Vec3& b) {
    return Near(a.x, b.x) && Near(a.y, b.y) && Near(a.z, b.z);
}

int main() {
    const fs::path dir = "/tmp/arc_scene_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const std::string file = (dir / "main.arc").string();

    // 1. Build scene A.
    Arc::Scene a;
    auto player = a.CreateEntity("Player");
    player.GetComponent<Arc::TransformComponent>().Position = {1.0f, 2.0f, 0.0f};
    auto enemy = a.CreateEntity("Enemy");
    enemy.GetComponent<Arc::TransformComponent>().Position = {-3.0f, 0.5f, 0.0f};
    enemy.GetComponent<Arc::TransformComponent>().Rotation = {0, 0, 1.57f};
    enemy.GetComponent<Arc::TransformComponent>().Scale = {0.7f, 0.7f, 1.0f};

    CHECK(Arc::SceneSerializer::Save(a, file), "save scene A");
    CHECK(fs::exists(file), "scene file written");

    // 2. Load into scene B while A still alive (instance pools MUST isolate).
    Arc::Scene b;
    CHECK(Arc::SceneSerializer::Load(file, b), "load scene B");
    CHECK(b.EntityCount() == 2, "entity count round-trips");
    auto ents = b.Entities();
    CHECK(ents.size() == 2 && ents[0].Name() == "Player" && ents[1].Name() == "Enemy",
          "names + deterministic creation order");
    CHECK(VecNear(ents[0].GetComponent<Arc::TransformComponent>().Position,
                  {1.0f, 2.0f, 0.0f}), "player position round-trips");
    CHECK(VecNear(ents[1].GetComponent<Arc::TransformComponent>().Position,
                  {-3.0f, 0.5f, 0.0f}), "enemy position round-trips");
    CHECK(VecNear(ents[1].GetComponent<Arc::TransformComponent>().Rotation,
                  {0.0f, 0.0f, 1.57f}), "enemy rotation round-trips");
    CHECK(VecNear(ents[1].GetComponent<Arc::TransformComponent>().Scale,
                  {0.7f, 0.7f, 1.0f}), "enemy scale round-trips");

    // 3. Isolation: mutating A must not touch B (static-pool bug regression).
    player.GetComponent<Arc::TransformComponent>().Position = {99.0f, 99.0f, 99.0f};
    CHECK(VecNear(ents[0].GetComponent<Arc::TransformComponent>().Position,
                  {1.0f, 2.0f, 0.0f}), "A/B pools isolated");

    // 4. Second round-trip: save B, load into fresh C.
    const std::string file2 = (dir / "again.arc").string();
    CHECK(Arc::SceneSerializer::Save(b, file2), "save B");
    Arc::Scene c;
    CHECK(Arc::SceneSerializer::Load(file2, c), "load C");
    CHECK(c.EntityCount() == 2, "C count");
    CHECK(VecNear(c.Entities()[1].GetComponent<Arc::TransformComponent>().Scale,
                  {0.7f, 0.7f, 1.0f}), "C scale round-trips");

    // 5. Failure paths return false (no crash).
    CHECK(!Arc::SceneSerializer::Load("/nonexistent/scene.arc", c), "missing file fails");
    { std::ofstream f(dir / "bad.arc"); f << "scene: [\n  {broken"; }
    CHECK(!Arc::SceneSerializer::Load((dir / "bad.arc").string(), c), "malformed YAML fails");

    fs::remove_all(dir);
    std::printf("test_scene: ALL OK\n");
    return 0;
}
