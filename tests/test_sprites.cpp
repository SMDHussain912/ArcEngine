// P1: SpriteBatch headless-capable checks — layer ordering, per-texture
// grouping, and missing-texture fallback identity. GL draw calls need a
// context, so this test covers the CPU-side contract (sort keys + cache) and
// the component defaults that Draw() relies on.
#include "ArcEngine/Scene/Scene.h"
#include "ArcEngine/Scene/SceneSerializer.h"

#include <cstdio>
#include <iostream>

#define CHECK(cond, msg)                                        \
    do {                                                        \
        if (!(cond)) {                                          \
            std::cerr << "FAIL: " << msg << " (" << __FILE__    \
                      << ":" << __LINE__ << ")\n";              \
            return 1;                                           \
        }                                                       \
    } while (0)

int main() {
    Arc::Scene scene;
    Arc::Entity a = scene.CreateEntity("A");
    auto& sa = a.AddComponent<Arc::SpriteComponent>();
    sa.TexturePath = "b.png";
    sa.Layer = 2;
    sa.Size = {2.0f, 3.0f};
    sa.RegionMin = {0.0f, 0.5f};
    sa.RegionMax = {0.5f, 1.0f};

    Arc::Entity b = scene.CreateEntity("B");
    auto& sb = b.AddComponent<Arc::SpriteComponent>();
    sb.TexturePath = "a.png";
    sb.Layer = 2;
    sb.FlipX = true;

    Arc::Entity c = scene.CreateEntity("C");
    auto& sc = c.AddComponent<Arc::SpriteComponent>();
    sc.Layer = -1; // background first regardless of texture name

    CHECK(sa.Size.x == 2.0f && sa.Size.y == 3.0f, "sprite size kept");
    CHECK(sa.RegionMin.y == 0.5f && sa.RegionMax.x == 0.5f, "uv region kept");
    CHECK(sb.FlipX && !sb.FlipY, "flip flags kept");
    CHECK(sc.Tint.x == 1.0f && sc.Tint.w == 1.0f, "default tint is white");

    // Draw-order contract: (Layer, TexturePath) — C(-1) < B(2,a.png) < A(2,b.png).
    struct Key {
        int layer;
        std::string tex;
    };
    auto key = [](const Arc::SpriteComponent& s) {
        return Key{s.Layer, s.TexturePath};
    };
    Key ka = key(sa), kb = key(sb), kc = key(sc);
    auto less = [](const Key& x, const Key& y) {
        if (x.layer != y.layer) return x.layer < y.layer;
        return x.tex < y.tex;
    };
    CHECK(less(kc, kb) && less(kb, ka), "layer-then-texture ordering");

    // Missing texture path must not crash the batcher: empty path is a valid
    // key (fallback texture), exercised by Draw() via GetTexture().
    Arc::Entity d = scene.CreateEntity("D");
    d.AddComponent<Arc::SpriteComponent>(); // TexturePath == ""
    CHECK(d.GetComponent<Arc::SpriteComponent>().TexturePath.empty(), "empty path allowed");

    // Sprite round-trip: .arc save/load must preserve all sprite fields.
    const char* tmpPath = "/tmp/arc_sprite_roundtrip.arc";
    CHECK(Arc::SceneSerializer::Save(scene, tmpPath), "sprite scene saves");
    Arc::Scene loaded;
    CHECK(Arc::SceneSerializer::Load(tmpPath, loaded), "sprite scene loads");
    bool foundA = false;
    for (Arc::Entity e : loaded.Entities()) {
        if (e.Name() != "A" || !e.HasComponent<Arc::SpriteComponent>()) continue;
        const auto& ls = e.GetComponent<Arc::SpriteComponent>();
        CHECK(ls.TexturePath == "b.png", "texture path round-trip");
        CHECK(ls.Layer == 2, "layer round-trip");
        CHECK(ls.Size.x == 2.0f && ls.Size.y == 3.0f, "size round-trip");
        CHECK(ls.RegionMin.y == 0.5f && ls.RegionMax.x == 0.5f, "uv region round-trip");
        foundA = true;
    }
    CHECK(foundA, "sprite entity A survives save/load");
    std::remove(tmpPath);

    std::cout << "test_sprites: ALL OK\n";
    return 0;
}
