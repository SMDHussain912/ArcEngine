#pragma once
#include <cstdint>
#include <string>

namespace Arc {

// Thin wrappers over a compiled shader program (M2).
// Usage:
//   Shader s(vertexSrc, fragmentSrc);
//   s.Bind(); s.SetFloat("u_Time", t); s.Unbind();
class Shader {
public:
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    void Bind() const;
    void Unbind() const;
    bool IsValid() const { return m_id != 0; }

    void SetFloat(const std::string& name, float v) const;
    void SetVec2(const std::string& name, float x, float y) const;
    void SetInt(const std::string& name, int v) const;

    static Shader FromFiles(const std::string& vertexPath, const std::string& fragmentPath);

private:
    static uint32_t Compile(uint32_t type, const std::string& src);
    static std::string ReadFile(const std::string& path);

private:
    uint32_t m_id = 0;
};

} // namespace Arc
