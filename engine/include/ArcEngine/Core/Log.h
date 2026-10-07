#pragma once
#include <string>

namespace Arc {

// Minimal colored console logger (M1). No external deps.
// Later: file sinks, Lua bindings, editor console.
class Log {
public:
    static void Init();
    static void Info(const std::string& msg);
    static void Warn(const std::string& msg);
    static void Error(const std::string& msg);
    static void Debug(const std::string& msg);
};

} // namespace Arc
