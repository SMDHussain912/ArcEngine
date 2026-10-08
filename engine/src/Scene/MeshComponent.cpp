#include "ArcEngine/Scene/MeshComponent.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/Shader.h"

#include <glad/gl.h>

namespace Arc {

namespace {
void PushTri(std::vector<float>& v, float ax, float ay, float bx, float by, float cx, float cy,
             float r, float g, float b) {
    v.insert(v.end(), {ax, ay, 0.0f, r, g, b, bx, by, 0.0f, r, g, b, cx, cy, 0.0f, r, g, b});
}
} // namespace

MeshComponent MeshComponent::Triangle(const float rgb[3]) {
    MeshComponent m;
    m.MeshId = "triangle";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    m.Vertices = {
         0.0f,  0.5f, 0.0f,  r, g, b,
        -0.5f, -0.5f, 0.0f,  r, g, b,
         0.5f, -0.5f, 0.0f,  r, g, b,
    };
    return m;
}

MeshComponent MeshComponent::Quad(const float rgb[3]) {
    MeshComponent m;
    m.MeshId = "quad";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    // Two triangles (6 verts), CCW
    m.Vertices = {
        -0.5f,  0.5f, 0.0f,  r, g, b,
        -0.5f, -0.5f, 0.0f,  r, g, b,
         0.5f, -0.5f, 0.0f,  r, g, b,
        -0.5f,  0.5f, 0.0f,  r, g, b,
         0.5f, -0.5f, 0.0f,  r, g, b,
         0.5f,  0.5f, 0.0f,  r, g, b,
    };
    return m;
}

MeshComponent MeshComponent::Circle(const float rgb[3], int segments) {
    MeshComponent m;
    m.MeshId = "circle";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    segments = std::max(6, segments);
    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        float a1 = 2.0f * 3.14159265f * static_cast<float>(i + 1) / static_cast<float>(segments);
        PushTri(m.Vertices, 0.0f, 0.0f, std::cos(a0) * 0.5f, std::sin(a0) * 0.5f,
                std::cos(a1) * 0.5f, std::sin(a1) * 0.5f, r, g, b);
    }
    return m;
}

MeshComponent MeshComponent::Ring(const float rgb[3], float inner, int segments) {
    MeshComponent m;
    m.MeshId = "ring";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    segments = std::max(8, segments);
    inner = std::min(0.9f, std::max(0.05f, inner));
    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        float a1 = 2.0f * 3.14159265f * static_cast<float>(i + 1) / static_cast<float>(segments);
        float c0 = std::cos(a0), s0 = std::sin(a0), c1 = std::cos(a1), s1 = std::sin(a1);
        PushTri(m.Vertices, c0 * inner, s0 * inner, c0 * 0.5f, s0 * 0.5f, c1 * 0.5f, s1 * 0.5f, r,
                g, b);
        PushTri(m.Vertices, c0 * inner, s0 * inner, c1 * 0.5f, s1 * 0.5f, c1 * inner, s1 * inner,
                r, g, b);
    }
    return m;
}

MeshComponent MeshComponent::Plane(const float rgb[3], float size) {
    MeshComponent m;
    m.MeshId = "plane";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    float h = size * 0.5f;
    m.Vertices = {
        -h, h, 0.0f, r, g, b, -h, -h, 0.0f, r, g, b, h, -h, 0.0f, r, g, b,
        -h, h, 0.0f, r, g, b, h, -h, 0.0f, r, g, b, h, h, 0.0f, r, g, b,
    };
    return m;
}

MeshComponent MeshComponent::Cross(const float rgb[3], float arm, float thick) {
    MeshComponent m;
    m.MeshId = "cross";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    float t = thick * 0.5f;
    auto quad = [&](float x0, float y0, float x1, float y1) {
        PushTri(m.Vertices, x0, y0, x0, y1, x1, y1, r, g, b);
        PushTri(m.Vertices, x0, y0, x1, y1, x1, y0, r, g, b);
    };
    quad(-t, -arm, t, arm); // vertical bar
    quad(-arm, -t, arm, t); // horizontal bar
    return m;
}

MeshComponent MeshComponent::Arrow(const float rgb[3], float len) {
    MeshComponent m;
    m.MeshId = "arrow";
    float r = rgb[0], g = rgb[1], b = rgb[2];
    float shaft = len * 0.65f, head = len - shaft, hw = len * 0.18f, sw = len * 0.06f;
    PushTri(m.Vertices, 0.0f, -sw, 0.0f, sw, shaft, sw, r, g, b);
    PushTri(m.Vertices, 0.0f, -sw, shaft, sw, shaft, -sw, r, g, b);
    PushTri(m.Vertices, shaft, -hw, shaft, hw, len, 0.0f, r, g, b);
    return m;
}

MeshComponent MeshComponent::FromId(const std::string& id, const float rgb[3]) {
    if (id == "quad") return Quad(rgb);
    if (id == "circle") return Circle(rgb);
    if (id == "ring") return Ring(rgb);
    if (id == "plane") return Plane(rgb);
    if (id == "cross") return Cross(rgb);
    if (id == "arrow") return Arrow(rgb);
    return Triangle(rgb);
}

void MeshComponent::Upload() {
    if (Uploaded || Vertices.empty()) return;
    Vao = std::make_shared<VertexArray>();
    Vao->Bind();
    Vbo = std::make_shared<VertexBuffer>(Vertices.data(), Vertices.size());
    Vbo->Bind();
    Vao->LayoutFloat(0, 3, 6 * sizeof(float), 0);
    Vao->LayoutFloat(1, 3, 6 * sizeof(float), 3 * sizeof(float));
    Vao->Unbind();
    Uploaded = true;
}

void DrawMesh(const MeshComponent& mesh, const Shader& shader, const TransformComponent& t) {
    if (!mesh.Uploaded || !mesh.Vao) return;
    shader.Bind();
    Mat4 m = t.GetMatrix();
    shader.SetMat4("u_Model", &m[0][0]);
    mesh.Vao->Bind();
    uint32_t count = static_cast<uint32_t>(mesh.Vertices.size() / 6);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(count));
}

} // namespace Arc
