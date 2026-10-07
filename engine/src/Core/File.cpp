#include "ArcEngine/Core/File.h"
#include "ArcEngine/Core/Log.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace Arc {

bool File::Exists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

std::string File::ReadText(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        Log::Error("File not found: " + path);
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::vector<uint8_t> File::ReadBinary(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        Log::Error("File not found: " + path);
        return {};
    }
    auto size = f.tellg();
    std::vector<uint8_t> data(static_cast<size_t>(size));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

bool File::WriteText(const std::string& path, const std::string& text) {
    std::ofstream f(path);
    if (!f) {
        Log::Error("Cannot write file: " + path);
        return false;
    }
    f << text;
    return true;
}

} // namespace Arc
