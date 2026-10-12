#include "ArcEngine/Renderer/SpriteBatch.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/Shader.h"
#include "ArcEngine/Renderer/Texture.h"
#include "ArcEngine/Scene/Scene.h"

#include <glad/gl.h>

#include <algorithm>

namespace Arc {

namespace {
// pos(3) + uv(2) + tint(4) = 9 floats per vertex, 4 verts per sprite.
constexpr size_t kFloatsPerVert = 9;
constexpr size_t kVertsPerQuad = 4;

const char* kVs = R"GLSL(#version 460 core
layout(location = 0) in vec3 a_Pos;
layout(location = 1) in vec2 a_UV;
layout(location = 2) in vec4 a_Tint;
out vec2 v_UV;
out vec4 v_Tint;
uniform mat4 u_ViewProj;
void main() {
    v_UV = a_UV;
    v_Tint = a_Tint;
    gl_Position = u_ViewProj * vec4(a_Pos, 1.0);
}
)GLSL";

const char* kFs = R"GLSL(#version 460 core
in vec2 v_UV;
in vec4 v_Tint;
out vec4 FragColor;
uniform sampler2D u_Tex;
void main() { FragColor = texture(u_Tex, v_UV) * v_Tint; }
)GLSL";
} // namespace

SpriteBatch::SpriteBatch() = default;

SpriteBatch::~SpriteBatch() {
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
} // GL handles are also released earlier via ReleaseGL() during editor shutdown

void SpriteBatch::ClearCache() {
    m_texCache.clear();
    m_lastBinds = 0;
    m_lastSprites = 0;
}

void SpriteBatch::ReleaseGL() {
    ClearCache();
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    m_ibo = m_vbo = m_vao = 0;
    m_vboCapVerts = m_iboCapQuads = 0;
    m_shader.reset();
    m_glReady = false;
}

std::shared_ptr<Texture> SpriteBatch::GetTexture(const std::string& path) {
    std::string key = path.empty() ? "<missing>" : path;
    auto it = m_texCache.find(key);
    if (it != m_texCache.end()) return it->second;
    auto tex = std::make_shared<Texture>(key == "<missing>" ? "__missing__" : key);
    m_texCache.emplace(key, tex);
    return tex;
}

void SpriteBatch::EnsureGL() {
    if (m_glReady) return;
    m_shader = std::make_unique<Shader>(kVs, kFs);
    if (!m_shader->IsValid()) {
        m_shader.reset();
        return;
    }
    GLuint vao = 0, vbo = 0, ibo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ibo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    constexpr size_t stride = kFloatsPerVert * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(5 * sizeof(float)));
    glBindVertexArray(0);
    m_vao = vao;
    m_vbo = vbo;
    m_ibo = ibo;
    m_glReady = true;
}

void SpriteBatch::UploadQuads(const std::vector<SpriteDraw>& draws, size_t begin, size_t end) {
    size_t quads = end - begin;
    size_t verts = quads * kVertsPerQuad;
    if (verts > m_vboCapVerts) {
        size_t cap = 1;
        while (cap < verts) cap *= 2;
        cap = std::max<size_t>(cap, 64);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, cap * kFloatsPerVert * sizeof(float), nullptr,
                     GL_STREAM_DRAW);
        m_vboCapVerts = cap;
    }
    std::vector<float> data;
    data.reserve(verts * kFloatsPerVert);
    for (size_t i = begin; i < end; ++i) {
        const SpriteDraw& d = draws[i];
        float hx = d.size.x * 0.5f, hy = d.size.y * 0.5f;
        Vec4 p0 = d.model * Vec4(-hx, -hy, 0.0f, 1.0f);
        Vec4 p1 = d.model * Vec4(hx, -hy, 0.0f, 1.0f);
        Vec4 p2 = d.model * Vec4(hx, hy, 0.0f, 1.0f);
        Vec4 p3 = d.model * Vec4(-hx, hy, 0.0f, 1.0f);
        float u0 = d.uvMin.x, v0 = d.uvMin.y, u1 = d.uvMax.x, v1 = d.uvMax.y;
        const Vec4 c = d.tint;
        data.insert(data.end(), {p0.x, p0.y, p0.z, u0, v0, c.x, c.y, c.z, c.w});
        data.insert(data.end(), {p1.x, p1.y, p1.z, u1, v0, c.x, c.y, c.z, c.w});
        data.insert(data.end(), {p2.x, p2.y, p2.z, u1, v1, c.x, c.y, c.z, c.w});
        data.insert(data.end(), {p3.x, p3.y, p3.z, u0, v1, c.x, c.y, c.z, c.w});
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, data.size() * sizeof(float), data.data());

    if (quads > m_iboCapQuads) {
        size_t cap = 1;
        while (cap < quads) cap *= 2;
        cap = std::max<size_t>(cap, 64);
        std::vector<uint32_t> idx;
        idx.reserve(cap * 6);
        for (size_t q = 0; q < cap; ++q) {
            uint32_t b = static_cast<uint32_t>(q * 4);
            idx.insert(idx.end(), {b, b + 1, b + 2, b, b + 2, b + 3});
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(uint32_t), idx.data(),
                     GL_STATIC_DRAW);
        m_iboCapQuads = cap;
    }
}

void SpriteBatch::Draw(Scene& scene, const Mat4& viewProj) {
    EnsureGL();
    m_lastBinds = 0;
    m_lastSprites = 0;
    if (!m_glReady || !m_shader) return;

    std::vector<SpriteDraw> draws;
    scene.Each<SpriteComponent>([&](Entity e, SpriteComponent& s) {
        SpriteDraw d;
        d.entityId = e.Id();
        d.layer = s.Layer;
        d.texPath = s.TexturePath;
        const TransformComponent& t = e.HasComponent<TransformComponent>()
                                          ? static_cast<const TransformComponent&>(
                                                e.GetComponent<TransformComponent>())
                                          : TransformComponent{};
        d.model = t.GetMatrix();
        d.tint = s.Tint;
        d.uvMin = s.RegionMin;
        d.uvMax = s.RegionMax;
        if (s.FlipX) std::swap(d.uvMin.x, d.uvMax.x);
        if (s.FlipY) std::swap(d.uvMin.y, d.uvMax.y);
        d.size = s.Size;
        draws.push_back(std::move(d));
    });
    if (draws.empty()) return;
    std::stable_sort(draws.begin(), draws.end(), [](const SpriteDraw& a, const SpriteDraw& b) {
        if (a.layer != b.layer) return a.layer < b.layer;
        return a.texPath < b.texPath;
    });
    m_lastSprites = static_cast<uint32_t>(draws.size());

    m_shader->Bind();
    m_shader->SetMat4("u_ViewProj", &viewProj[0][0]);
    m_shader->SetInt("u_Tex", 0);
    GLboolean wasBlend = glIsEnabled(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    size_t begin = 0;
    while (begin < draws.size()) {
        size_t end = begin + 1;
        while (end < draws.size() && draws[end].texPath == draws[begin].texPath) ++end;
        UploadQuads(draws, begin, end);
        GetTexture(draws[begin].texPath)->Bind(0);
        m_lastBinds++;
        uint32_t idxCount = static_cast<uint32_t>((end - begin) * 6);
        glDrawElements(GL_TRIANGLES, static_cast<int>(idxCount), GL_UNSIGNED_INT, nullptr);
        begin = end;
    }
    glBindVertexArray(0);
    if (wasBlend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}
} // namespace Arc
