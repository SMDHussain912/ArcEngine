#pragma once
#include "ArcEngine/Core/Math.h"

namespace Arc {

// Position + rotation (euler radians) + scale (M5).
// GetMatrix() = Translate * Rotate(ZYX) * Scale — Unity Transform feel.
// M7 editor will edit these fields in Inspector.
struct TransformComponent {
    Vec3 Position{0.0f, 0.0f, 0.0f};
    Vec3 Rotation{0.0f, 0.0f, 0.0f}; // radians, XYZ euler
    Vec3 Scale{1.0f, 1.0f, 1.0f};

    Mat4 GetMatrix() const {
        Mat4 m(1.0f);
        m = glm::translate(m, Position);
        m = glm::rotate(m, Rotation.z, glm::vec3(0, 0, 1));
        m = glm::rotate(m, Rotation.y, glm::vec3(0, 1, 0));
        m = glm::rotate(m, Rotation.x, glm::vec3(1, 0, 0));
        m = glm::scale(m, Scale);
        return m;
    }
};

} // namespace Arc

