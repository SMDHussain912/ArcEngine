#pragma once
#include "ArcEngine/Scene/Components.h"

#include <cstdint>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Arc {

class Scene;

// Entity = lightweight handle into Scene storage (M5).
// Not a GameObject pointer — copying an Entity is cheap and safe.
// Usage: Entity e = scene.CreateEntity("Player");
//        e.AddComponent<TransformComponent>().Position = {1,0,0};
//        if (e.HasComponent<TransformComponent>()) { ... }
class Entity {
public:
    Entity() = default;
    Entity(Scene* scene, uint32_t id);

    bool Valid() const;
    uint32_t Id() const { return m_id; }
    const std::string& Name() const;

    template <typename T, typename... Args>
    T& AddComponent(Args&&... args);

    template <typename T>
    bool HasComponent() const;

    template <typename T>
    T& GetComponent();

    template <typename T>
    const T& GetComponent() const;

    bool operator==(const Entity& o) const { return m_scene == o.m_scene && m_id == o.m_id; }

private:
    Scene* m_scene = nullptr;
    uint32_t m_id = 0;
};

} // namespace Arc
