#pragma once
// ArcEngine Math (M3a) — thin aliases + helpers over vendored/system glm.
// Why wrap instead of #include <glm/...> everywhere?
//  1. One place to swap math lib later if needed.
//  2. Unity-like helpers (Lerp, Degrees, Perspective) in Arc:: namespace.
//  3. Keeps gameplay code clean: Arc::Vec3, Arc::Mat4.
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Arc {

using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;
using Quat = glm::quat;

inline Mat4 Translate(const Mat4& m, const Vec3& v) { return glm::translate(m, v); }
inline Mat4 Rotate(const Mat4& m, float radians, const Vec3& axis) {
    return glm::rotate(m, radians, axis);
}
inline Mat4 Scale(const Mat4& m, const Vec3& v) { return glm::scale(m, v); }
inline Mat4 Perspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
    return glm::perspective(fovRadians, aspect, nearPlane, farPlane);
}
inline Mat4 Ortho(float l, float r, float b, float t, float n = -1.0f, float f = 1.0f) {
    return glm::ortho(l, r, b, t, n, f);
}
inline Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    return glm::lookAt(eye, center, up);
}

constexpr float Pi = 3.14159265358979323846f;
inline float Radians(float degrees) { return glm::radians(degrees); }
inline float Degrees(float radians) { return glm::degrees(radians); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return glm::mix(a, b, t); }
inline float Clamp(float v, float lo, float hi) { return glm::clamp(v, lo, hi); }

} // namespace Arc
