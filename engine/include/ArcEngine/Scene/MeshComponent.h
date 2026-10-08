#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Arc {

class Shader;
class VertexArray;
class VertexBuffer;

// Colored mesh data (M5) — CPU side.
// Renderer builds GPU buffers from this once (MeshComponent::Upload).
struct MeshComponent {
    std::vector<float> Vertices; // interleaved pos(3) + color(3)
    bool Uploaded = false;
    // Editor/game mesh identity (for .arc files + primitive menu).
    // Built-ins: triangle, quad, circle, ring, grid-plane, cross, arrow.
    // 3D path (M10+): gltf:<path> / prim:cube|sphere — stubs reserved here.
    std::string MeshId = "triangle";

    // GPU handles owned here (simple for M5; M6 moves to asset system).
    std::shared_ptr<VertexArray> Vao;
    std::shared_ptr<VertexBuffer> Vbo;

    static MeshComponent Triangle(const float rgb[3]);
    static MeshComponent Quad(const float rgb[3]);
    static MeshComponent Circle(const float rgb[3], int segments = 24);
    static MeshComponent Ring(const float rgb[3], float inner = 0.35f, int segments = 32);
    static MeshComponent Plane(const float rgb[3], float size = 4.0f);
    static MeshComponent Cross(const float rgb[3], float arm = 0.5f, float thick = 0.08f);
    static MeshComponent Arrow(const float rgb[3], float len = 0.9f);
    static MeshComponent FromId(const std::string& id, const float rgb[3]);

    void Upload();
};

// Per-entity draw helper: binds VAO + shader, sets u_Model, draws.
void DrawMesh(const MeshComponent& mesh, const Shader& shader, const struct TransformComponent& t);

} // namespace Arc
