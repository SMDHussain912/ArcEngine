#include "ArcEngine/Assets/AssetManager.h"
#include "ArcEngine/Core/File.h"
#include "ArcEngine/Core/Log.h"

#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>

namespace Arc {

namespace {

std::string Trim(const std::string& s) {
    const char* ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// `key = value` line parser shared by sidecar files (same family as Project).
bool FindKV(const std::string& line, std::string& key, std::string& val) {
    const std::string t = Trim(line);
    if (t.empty() || t[0] == '#') return false;
    const auto eq = t.find('=');
    if (eq == std::string::npos) return false;
    key = Trim(t.substr(0, eq));
    val = Trim(t.substr(eq + 1));
    return !key.empty();
}

} // namespace

void AssetManager::AddSearchPath(std::string path) {
    if (path.empty()) return;
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    m_searchPaths.push_back(std::move(path));
}

std::string AssetManager::Resolve(const std::string& relPath) const {
    if (relPath.empty()) return {};
    for (const auto& sp : m_searchPaths) {
        std::string cand = (sp == ".") ? relPath : sp + "/" + relPath;
        if (File::Exists(cand)) return cand;
    }
    if (File::Exists(relPath)) return relPath; // absolute or cwd-relative as-given
    return {};
}

std::string AssetManager::NewUUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t hi = dist(gen);
    uint64_t lo = dist(gen);
    // v4 layout: set version (4) + variant (10xx) bits.
    hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    char buf[37];
    std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%012llx",
                  static_cast<unsigned>(hi >> 32),
                  static_cast<unsigned>((hi >> 16) & 0xFFFF),
                  static_cast<unsigned>(hi & 0xFFFF),
                  static_cast<unsigned>((lo >> 48) & 0xFFFF),
                  static_cast<unsigned long long>(lo & 0xFFFFFFFFFFFFULL));
    return buf;
}

uint64_t AssetManager::HashBytes(const uint8_t* data, size_t n) {
    uint64_t h = 14695981039346656037ULL; // FNV-1a 64 offset basis
    for (size_t i = 0; i < n; i++) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

std::string AssetManager::HashFile(const std::string& absPath) {
    if (absPath.empty()) return {};
    std::ifstream f(absPath, std::ios::binary);
    if (!f) return {};
    std::ostringstream ss;
    ss << f.rdbuf();
    const std::string bytes = ss.str();
    uint64_t h = HashBytes(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    char buf[19];
    std::snprintf(buf, sizeof(buf), "0x%016llx", static_cast<unsigned long long>(h));
    return buf;
}

bool AssetManager::ReadSidecar(const std::string& sidecar, std::string& uuid, std::string& hash) {
    if (!File::Exists(sidecar)) return false;
    std::ifstream f(sidecar);
    if (!f) return false;
    std::string line;
    bool gotUuid = false;
    while (std::getline(f, line)) {
        std::string k, v;
        if (!FindKV(line, k, v)) continue;
        if (k == "uuid") { uuid = v; gotUuid = true; }
        else if (k == "source_hash") hash = v;
    }
    return gotUuid && !uuid.empty();
}

bool AssetManager::WriteSidecar(const std::string& sidecar, const std::string& uuid,
                                const std::string& hash) {
    std::ofstream f(sidecar);
    if (!f) {
        Log::Error("AssetManager: cannot write sidecar " + sidecar);
        return false;
    }
    f << "# ArcEngine asset import cache (generated — commit this file)\n"
      << "uuid = " << uuid << "\n"
      << "source_hash = " << hash << "\n";
    return true;
}

std::string AssetManager::EnsureRegistered(const std::string& relPath) {
    const std::string abs = Resolve(relPath);
    if (abs.empty()) {
        Log::Error("AssetManager: asset not found: " + relPath);
        return {};
    }

    auto it = m_byRel.find(relPath);
    if (it != m_byRel.end()) return it->second.uuid;

    const std::string sidecar = SidecarPath(abs);
    std::string uuid, hash;
    if (!ReadSidecar(sidecar, uuid, hash)) {
        uuid = NewUUID();
        hash = HashFile(abs);
        if (!WriteSidecar(sidecar, uuid, hash)) return {};
        Log::Info("AssetManager: registered '" + relPath + "' -> " + uuid);
    }

    Entry e{abs, uuid};
    m_byRel.emplace(relPath, e);
    m_byUuid.emplace(uuid, relPath);
    return uuid;
}

std::string AssetManager::GetUUID(const std::string& relPath) {
    return EnsureRegistered(relPath);
}

std::string AssetManager::PathForUUID(const std::string& uuid) const {
    auto it = m_byUuid.find(uuid);
    return it != m_byUuid.end() ? it->second : std::string{};
}

bool AssetManager::NeedsReimport(const std::string& relPath) {
    const std::string abs = Resolve(relPath);
    if (abs.empty()) return false;
    std::string uuid, cachedHash;
    if (!ReadSidecar(SidecarPath(abs), uuid, cachedHash)) return true;
    const std::string current = HashFile(abs);
    if (current.empty()) return true;
    return current != cachedHash;
}

void AssetManager::MarkImported(const std::string& relPath) {
    if (EnsureRegistered(relPath).empty()) return;
    const std::string abs = Resolve(relPath);
    const std::string sidecar = SidecarPath(abs);
    std::string uuid, hash;
    ReadSidecar(sidecar, uuid, hash);           // uuid exists (just ensured)
    WriteSidecar(sidecar, uuid, HashFile(abs)); // refresh hash
}

std::string AssetManager::LoadText(const std::string& relPath) {
    const std::string abs = Resolve(relPath);
    if (abs.empty()) {
        Log::Error("AssetManager: cannot load, not found: " + relPath);
        return {};
    }
    return File::ReadText(abs);
}

std::vector<uint8_t> AssetManager::LoadBinary(const std::string& relPath) {
    const std::string abs = Resolve(relPath);
    if (abs.empty()) {
        Log::Error("AssetManager: cannot load, not found: " + relPath);
        return {};
    }
    return File::ReadBinary(abs);
}

} // namespace Arc
