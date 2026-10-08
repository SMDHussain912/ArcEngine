#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Arc {

// Asset registry + path resolution (P0 Step 3).
//
// - Search paths: relative asset paths resolve against them in order
//   (project dir first, cwd fallback). Lives HERE rather than in File on
//   purpose: File stays a dumb filesystem helper (predictable in tests),
//   the resolver belongs to the asset system.
// - UUID registry: every registered asset gets a stable 128-bit id stored in
//   a sidecar `<source>.arc.import` next to the source (Godot-style .import).
//   Sidecars are meant to be committed to git.
// - Import cache: sidecar also stores a content hash. NeedsReimport() tells
//   typed importers (Texture P1, Audio, Model P2) when the source changed;
//   they call MarkImported() after writing artifacts.
// - LoadText/LoadBinary are sync raw loaders. Typed importers arrive with
//   their subsystems — this class owns resolution + identity only for now.
class AssetManager {
public:
    void AddSearchPath(std::string path);
    const std::vector<std::string>& SearchPaths() const { return m_searchPaths; }

    // First existing `<searchPath>/<rel>`; falls back to rel as-given.
    // "" when nothing found.
    std::string Resolve(const std::string& relPath) const;

    // Stable uuid for an asset. Creates the sidecar (with current content
    // hash) on first touch and registers it in-memory. "" if source missing.
    std::string GetUUID(const std::string& relPath);

    // Reverse lookup — only paths registered in THIS session (full scan for
    // the editor asset browser comes in P3).
    std::string PathForUUID(const std::string& uuid) const;

    // True when sidecar is missing or source content hash differs.
    bool NeedsReimport(const std::string& relPath);

    // Importers call this after writing artifacts — refreshes cached hash.
    void MarkImported(const std::string& relPath);

    std::string LoadText(const std::string& relPath);
    std::vector<uint8_t> LoadBinary(const std::string& relPath);

    static std::string NewUUID();  // v4-style 8-4-4-4-12 hex
    static uint64_t HashBytes(const uint8_t* data, size_t n);   // FNV-1a 64
    static std::string HashFile(const std::string& absPath);    // "0x..." or ""

private:
    struct Entry {
        std::string absPath;
        std::string uuid;
    };

    static std::string SidecarPath(const std::string& absPath) {
        return absPath + ".arc.import";
    }
    static bool ReadSidecar(const std::string& sidecar, std::string& uuid, std::string& hash);
    static bool WriteSidecar(const std::string& sidecar, const std::string& uuid,
                             const std::string& hash);

    // Resolve + read-or-create sidecar; registers in-memory. "" on failure.
    std::string EnsureRegistered(const std::string& relPath);

    std::vector<std::string> m_searchPaths;
    std::unordered_map<std::string, Entry> m_byRel;
    std::unordered_map<std::string, std::string> m_byUuid; // uuid -> rel
};

} // namespace Arc
