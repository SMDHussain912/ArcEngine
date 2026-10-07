#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

namespace Arc {

// GPU vertex buffer (M2). Owns a GL_ARRAY_BUFFER handle.
class VertexBuffer {
public:
    VertexBuffer(const float* data, size_t floatCount);
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    void Bind() const;
    void Unbind() const;

private:
    uint32_t m_id = 0;
};

// GPU index buffer (M2). Owns a GL_ELEMENT_ARRAY_BUFFER handle.
class IndexBuffer {
public:
    IndexBuffer(const uint32_t* data, size_t indexCount);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    void Bind() const;
    void Unbind() const;
    size_t Count() const { return m_count; }

private:
    uint32_t m_id = 0;
    size_t m_count = 0;
};

// Vertex array: describes layout (location, size, stride, offset) for Core profile.
// Core 4.6 REQUIRES a VAO — raw VBO draws from M1 tutorials will not work.
class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void Bind() const;
    void Unbind() const;

    // Example: layoutFloat(0, 3, stride=6*sizeof(float), offset=0) for position
    void LayoutFloat(uint32_t location, int size, size_t stride, size_t offset);

private:
    uint32_t m_id = 0;
};

} // namespace Arc
