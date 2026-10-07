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

    // GPU handles owned here (simple for M5; M6 moves to asset system).
    std::shared_ptr<VertexArray> Vao;
    std::shared_ptr<VertexBuffer> Vbo;

    static MeshComponent Triangle(const float rgb[3]);
    static MeshComponent Quad(const float rgb[3]);

    void Upload();
};

// Per-entity draw helper: binds VAO + shader, sets u_Model, draws.
void DrawMesh(const MeshComponent& mesh, const Shader& shader, const struct TransformComponent& t);

} // namespace Arc
