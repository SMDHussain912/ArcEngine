#pragma once
#include "ArcEngine/Core/Math.h"
#include "ArcEngine/Scene/Components.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Arc {

class Scene;
class Shader;
class Texture;
struct Entity;

// SpriteBatch (P1): Godot-style 2D sprite renderer. Collects every
// SpriteComponent in a Scene, sorts by (Layer, TexturePath), and emits one
// indexed draw per texture bind — instead of 06-texture's one-VAO-per-sprite.
// Dynamic streaming VBO (orphan + subdata) keeps per-frame CPU upload cheap.
// GLES path (M8) uses the same buffers; only the backend draw call differs.
class SpriteBatch {
public:
    SpriteBatch();
    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    // Draw all sprites in scene with the given ortho view-projection.
    // Missing textures fall back to magenta (Texture's own fallback).
    void Draw(Scene& scene, const Mat4& viewProj);

    // Test/overlay aid: how many texture binds the last Draw issued.
    uint32_t LastBindCount() const { return m_lastBinds; }
    uint32_t LastSpriteCount() const { return m_lastSprites; }

    void ClearCache(); // drop uploaded textures (hot-reload / tests)

    // Free GL buffers/shader/textures while the context is still current.
    // Call from editor shutdown; Draw() lazily re-creates everything.
    void ReleaseGL();

private:
    struct SpriteDraw {
        uint32_t entityId = 0;
        int32_t layer = 0;
        std::string texPath;
        Mat4 model{1.0f};
        Vec4 tint{1, 1, 1, 1};
        Vec2 uvMin{0, 0};
        Vec2 uvMax{1, 1};
        Vec2 size{1, 1};
    };

    std::shared_ptr<Texture> GetTexture(const std::string& path);

    void EnsureGL();
    void UploadQuads(const std::vector<SpriteDraw>& draws, size_t begin, size_t end);

    std::unique_ptr<Shader> m_shader;
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ibo = 0;
    size_t m_vboCapVerts = 0; // streamed vertex capacity
    size_t m_iboCapQuads = 0; // indexed quad capacity
    bool m_glReady = false;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_texCache;
    uint32_t m_lastBinds = 0;
    uint32_t m_lastSprites = 0;
};

} // namespace Arc
