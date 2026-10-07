#include "ArcEngine/Renderer/Shader.h"
#include "ArcEngine/Core/Log.h"

#include <glad/gl.h>
#include <fstream>
#include <sstream>
#include <utility>

namespace Arc {

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc) {
    uint32_t vs = Compile(GL_VERTEX_SHADER, vertexSrc);
    uint32_t fs = Compile(GL_FRAGMENT_SHADER, fragmentSrc);
    if (vs == 0 || fs == 0) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vs);
    glAttachShader(m_id, fs);
    glLinkProgram(m_id);

    int ok = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        Log::Error(std::string("Shader link failed: ") + log);
        glDeleteProgram(m_id);
        m_id = 0;
    } else {
        Log::Info("Shader program linked.");
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

Shader::Shader(Shader&& other) noexcept : m_id(other.m_id) {
    other.m_id = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_id) glDeleteProgram(m_id);
        m_id = other.m_id;
        other.m_id = 0;
    }
    return *this;
}

void Shader::Bind() const {
    if (m_id) glUseProgram(m_id);
}

void Shader::Unbind() const {
    glUseProgram(0);
}

void Shader::SetFloat(const std::string& name, float v) const {
    int loc = glGetUniformLocation(m_id, name.c_str());
    if (loc >= 0) glUniform1f(loc, v);
}

void Shader::SetInt(const std::string& name, int v) const {
    int loc = glGetUniformLocation(m_id, name.c_str());
    if (loc >= 0) glUniform1i(loc, v);
}

uint32_t Shader::Compile(uint32_t type, const std::string& src) {
    uint32_t id = glCreateShader(type);
    const char* s = src.c_str();
    glShaderSource(id, 1, &s, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = {};
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        Log::Error(std::string(type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") +
                   " compile failed: " + log);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

std::string Shader::ReadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        Log::Error("Shader file not found: " + path);
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

Shader Shader::FromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    return Shader(ReadFile(vertexPath), ReadFile(fragmentPath));
}

} // namespace Arc
