#include "ArcEngine/Renderer/Texture.h"
#include "ArcEngine/Core/Log.h"

#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace Arc {

Texture::Texture(const std::string& path, bool flipY) : m_path(path) {
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int w = 0, h = 0, ch = 0;
    uint8_t* pixels = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!pixels) {
        Log::Error("Texture load failed: " + path + " (" + stbi_failure_reason() + ")");
        // magenta checker fallback so missing assets are OBVIOUS on screen
        static uint8_t fallback[4 * 4 * 4];
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++) {
                bool pink = ((x + y) % 2) == 0;
                uint8_t* p = fallback + (y * 4 + x) * 4;
                p[0] = pink ? 255 : 0; p[1] = 0; p[2] = pink ? 255 : 0; p[3] = 255;
            }
        m_width = 4; m_height = 4; m_channels = 4;
        Upload(fallback);
        return;
    }
    m_width = static_cast<uint32_t>(w);
    m_height = static_cast<uint32_t>(h);
    m_channels = 4;
    Upload(pixels);
    stbi_image_free(pixels);
    Log::Info("Texture loaded: " + path + " (" + std::to_string(m_width) + "x" +
              std::to_string(m_height) + ")");
}

Texture::Texture(uint32_t width, uint32_t height, const uint8_t* rgba)
    : m_width(width), m_height(height), m_channels(4), m_path("<procedural>") {
    Upload(rgba);
}

Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

void Texture::Upload(const uint8_t* rgba) {
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<int>(m_width),
                 static_cast<int>(m_height), 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace Arc
