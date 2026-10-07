#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace Arc {

// Tiny file helpers (M3c) — text + binary, no <filesystem> leak into callers.
// Paths are relative to cwd (run from project root). M6 adds asset search paths.
class File {
public:
    static bool Exists(const std::string& path);
    static std::string ReadText(const std::string& path);
    static std::vector<uint8_t> ReadBinary(const std::string& path);
    static bool WriteText(const std::string& path, const std::string& text);
};

} // namespace Arc
