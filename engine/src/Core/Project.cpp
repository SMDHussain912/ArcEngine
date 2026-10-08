#include "ArcEngine/Core/Project.h"
#include "ArcEngine/Core/File.h"
#include "ArcEngine/Core/Log.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

namespace Arc {

namespace {

std::string Trim(const std::string& s) {
    const char* ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return {};
    auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// Strict parse: whole string must consume. Logs + returns fallback on junk.
uint32_t ParseU32(const std::string& v, uint32_t fallback, const std::string& where) {
    char* end = nullptr;
    unsigned long n = std::strtoul(v.c_str(), &end, 10);
    if (end == v.c_str() || *end != '\0') {
        Log::Warn("Project: bad integer '" + v + "' for " + where + ", keeping default");
        return fallback;
    }
    return static_cast<uint32_t>(n);
}

float ParseF32(const std::string& v, float fallback, const std::string& where) {
    char* end = nullptr;
    float f = std::strtof(v.c_str(), &end);
    if (end == v.c_str() || *end != '\0' || f <= 0.0f) {
        Log::Warn("Project: bad number '" + v + "' for " + where + ", keeping default");
        return fallback;
    }
    return f;
}

bool ParseBool(const std::string& v, bool fallback, const std::string& where) {
    if (v == "true" || v == "1" || v == "yes") return true;
    if (v == "false" || v == "0" || v == "no") return false;
    Log::Warn("Project: bad bool '" + v + "' for " + where + ", keeping default");
    return fallback;
}

} // namespace

std::optional<Project> Project::Load(const std::string& path) {
    if (!File::Exists(path)) {
        Log::Error("Project file not found: " + path);
        return std::nullopt;
    }
    const std::string text = File::ReadText(path);
    if (text.empty()) {
        Log::Error("Project file is empty or unreadable: " + path);
        return std::nullopt;
    }

    Project p;
    p.FilePath = path;
    std::string section;
    std::istringstream ss(text);
    std::string line;
    int lineNo = 0;

    while (std::getline(ss, line)) {
        lineNo++;
        const std::string t = Trim(line);
        if (t.empty() || t[0] == '#') continue;

        if (t[0] == '[') {
            if (t.back() != ']') {
                Log::Warn(path + ":" + std::to_string(lineNo) + " malformed section: " + t);
                continue;
            }
            section = t.substr(1, t.size() - 2);
            continue;
        }

        const auto eq = t.find('=');
        if (eq == std::string::npos) {
            Log::Warn(path + ":" + std::to_string(lineNo) + " missing '=': " + t);
            continue;
        }
        const std::string key = Trim(t.substr(0, eq));
        const std::string val = Trim(t.substr(eq + 1));
        const std::string where = section + "." + key;

        if (section == "project") {
            if (key == "name") p.Name = val;
            else if (key == "version") p.Version = val;
            else if (key == "main_scene") p.MainScene = val;
            else Log::Warn(path + ":" + std::to_string(lineNo) + " unknown key: " + where);
        } else if (section == "window") {
            if (key == "title") p.WindowTitle = val;
            else if (key == "width") p.Width = ParseU32(val, p.Width, where);
            else if (key == "height") p.Height = ParseU32(val, p.Height, where);
            else if (key == "vsync") p.VSync = ParseBool(val, p.VSync, where);
            else if (key == "physics_hz") p.PhysicsHz = ParseF32(val, p.PhysicsHz, where);
            else Log::Warn(path + ":" + std::to_string(lineNo) + " unknown key: " + where);
        } else {
            Log::Warn(path + ":" + std::to_string(lineNo) + " key outside known section: " + where);
        }
    }

    Log::Info("Project loaded: '" + p.Name + "' v" + p.Version + " (" + path + ")");
    return p;
}

bool Project::Save(const std::string& path) const {
    std::ofstream f(path);
    if (!f) {
        Log::Error("Project: cannot write " + path);
        return false;
    }
    f << "# ArcEngine project descriptor (generated)\n"
      << "[project]\n"
      << "name = " << Name << "\n"
      << "version = " << Version << "\n"
      << "main_scene = " << MainScene << "\n"
      << "\n[window]\n"
      << "title = " << WindowTitle << "\n"
      << "width = " << Width << "\n"
      << "height = " << Height << "\n"
      << "vsync = " << (VSync ? "true" : "false") << "\n"
      << "physics_hz = " << PhysicsHz << "\n";
    return true;
}

AppConfig Project::ToAppConfig() const {
    AppConfig cfg;
    cfg.Title = WindowTitle.empty() ? Name : WindowTitle;
    cfg.Width = Width;
    cfg.Height = Height;
    cfg.VSync = VSync;
    cfg.PhysicsHz = PhysicsHz;
    return cfg;
}

} // namespace Arc
