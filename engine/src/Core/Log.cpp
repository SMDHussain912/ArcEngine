#include "ArcEngine/Core/Log.h"
#include <iostream>

namespace Arc {

namespace {
const char* kReset  = "\033[0m";
const char* kGreen  = "\033[32m";
const char* kYellow = "\033[33m";
const char* kRed    = "\033[31m";
const char* kGray   = "\033[90m";
bool s_color = true;
} // namespace

void Log::Init() {
    // Disable colors when not a TTY (pipes, CI logs).
    s_color = (std::getenv("NO_COLOR") == nullptr);
}

void Log::Info(const std::string& msg) {
    std::cout << (s_color ? kGreen : "") << "[INFO] " << (s_color ? kReset : "") << msg << "\n";
}

void Log::Warn(const std::string& msg) {
    std::cout << (s_color ? kYellow : "") << "[WARN] " << (s_color ? kReset : "") << msg << "\n";
}

void Log::Error(const std::string& msg) {
    std::cerr << (s_color ? kRed : "") << "[ERROR]" << (s_color ? kReset : "") << " " << msg << "\n";
}

void Log::Debug(const std::string& msg) {
#ifdef NDEBUG
    (void)msg;
#else
    std::cout << (s_color ? kGray : "") << "[DEBUG]" << (s_color ? kReset : "") << " " << msg << "\n";
#endif
}

} // namespace Arc
