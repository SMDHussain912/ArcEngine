// P0 headless test: AssetManager search paths, UUID sidecars, hash cache.
// Run from the REPO ROOT: ./build/tests/test_asset_manager
// Uses /tmp/arc_am_test as a scratch asset dir (cleaned up at the end).
#include "ArcEngine/Assets/AssetManager.h"

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

int main() {
    const fs::path dir = "/tmp/arc_am_test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    { std::ofstream f(dir / "a.txt"); f << "hello assets"; }

    Arc::AssetManager am;
    am.AddSearchPath(dir.string());

    // 1. Resolve via search path.
    const std::string rel = "a.txt";
    CHECK(am.Resolve(rel) == (dir / "a.txt").string(), "resolve via search path");
    CHECK(am.Resolve("missing.file").empty(), "missing resolves to empty");

    // 2. UUID creation + sidecar.
    const std::string uuid1 = am.GetUUID(rel);
    CHECK(!uuid1.empty(), "uuid created");
    CHECK(fs::exists(dir / "a.txt.arc.import"), "sidecar written next to source");
    CHECK(am.GetUUID(rel) == uuid1, "uuid stable within session");
    CHECK(!am.NeedsReimport(rel), "fresh registration = no reimport");
    CHECK(am.PathForUUID(uuid1) == rel, "reverse lookup registered path");

    // 3. UUID survives a brand-new manager (sidecar on disk).
    Arc::AssetManager am2;
    am2.AddSearchPath(dir.string());
    CHECK(am2.GetUUID(rel) == uuid1, "uuid stable across instances");
    CHECK(!am2.NeedsReimport(rel), "cached hash matches unchanged source");

    // 4. Source change detection + MarkImported.
    { std::ofstream f(dir / "a.txt"); f << "changed!"; }
    CHECK(am2.NeedsReimport(rel), "modified source detected");
    am2.MarkImported(rel);
    CHECK(!am2.NeedsReimport(rel), "MarkImported refreshes cached hash");

    // 5. Loaders + miss handling.
    CHECK(am2.LoadText(rel) == "changed!", "LoadText resolves through search path");
    CHECK(am2.LoadText("missing.file").empty(), "LoadText miss returns empty");
    CHECK(am2.GetUUID("missing.file").empty(), "GetUUID miss returns empty");
    CHECK(!am2.NeedsReimport("missing.file"), "NeedsReimport miss returns false");

    fs::remove_all(dir);
    std::printf("test_asset_manager: ALL OK\n");
    return 0;
}
