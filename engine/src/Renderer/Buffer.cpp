#include "ArcEngine/Renderer/Buffer.h"
#include <glad/gl.h>

namespace Arc {

VertexBuffer::VertexBuffer(const float* data, size_t floatCount) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(floatCount * sizeof(float)),
                 data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
    if (m_id) glDeleteBuffers(1, &m_id);
}

void VertexBuffer::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_id); }
void VertexBuffer::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

IndexBuffer::IndexBuffer(const uint32_t* data, size_t indexCount) : m_count(indexCount) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indexCount * sizeof(uint32_t)),
                 data, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    if (m_id) glDeleteBuffers(1, &m_id);
}

void IndexBuffer::Bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id); }
void IndexBuffer::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

VertexArray::VertexArray() {
    glGenVertexArrays(1, &m_id);
}

VertexArray::~VertexArray() {
    if (m_id) glDeleteVertexArrays(1, &m_id);
}

void VertexArray::Bind() const { glBindVertexArray(m_id); }
void VertexArray::Unbind() const { glBindVertexArray(0); }

void VertexArray::LayoutFloat(uint32_t location, int size, size_t stride, size_t offset) {
    Bind();
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE,
                          static_cast<GLsizei>(stride),
                          reinterpret_cast<const void*>(offset));
}

} // namespace Arc
