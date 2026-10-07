#pragma once
#include <cstdint>
#include <string>

namespace Arc {

// 2D texture (M6a) — loads PNG/JPG via stb_image, uploads to GL.
// Usage: Texture t("assets/textures/logo.png"); t.Bind(0);
class Texture {
public:
    explicit Texture(const std::string& path, bool flipY = true);
    // Procedural fallback: solid color texture (used when file missing).
    Texture(uint32_t width, uint32_t height, const uint8_t* rgba);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    bool IsValid() const { return m_id != 0; }
    uint32_t Width() const { return m_width; }
    uint32_t Height() const { return m_height; }
    const std::string& Path() const { return m_path; }

private:
    void Upload(const uint8_t* rgba);

private:
    uint32_t m_id = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    int m_channels = 0;
    std::string m_path;
};

} // namespace Arc
