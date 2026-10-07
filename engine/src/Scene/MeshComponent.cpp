#include "ArcEngine/Scene/MeshComponent.h"
#include "ArcEngine/Scene/Components.h"
#include "ArcEngine/Core/Log.h"
#include "ArcEngine/Renderer/Buffer.h"
#include "ArcEngine/Renderer/Shader.h"

#include <glad/gl.h>

namespace Arc {

MeshComponent MeshComponent::Triangle(const float rgb[3]) {
    MeshComponent m;
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
