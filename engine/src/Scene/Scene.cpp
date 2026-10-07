#include "ArcEngine/Scene/Scene.h"

namespace Arc {

Entity Scene::CreateEntity(const std::string& name) {
    uint32_t id = m_nextId++;
    m_names[id] = name;
    m_alive[id] = true;
    // Every entity starts with a Transform (Unity-like default).
    AddComponent<TransformComponent>(id);
    return Entity(this, id);
}

void Scene::DestroyEntity(Entity e) {
    if (!e.Valid()) return;
    m_alive[e.Id()] = false;
    m_names.erase(e.Id());
    // NOTE: static component pools keep stale entries for M5 simplicity.
    // Destroyed ids never get reused (m_nextId only grows), so Each() skips via m_alive.
}

void Scene::Clear() {
    m_names.clear();
    m_alive.clear();
    m_nextId = 1;
}

std::vector<Entity> Scene::Entities() {
    std::vector<Entity> out;
    for (auto& [id, alive] : m_alive) {
        if (alive) out.emplace_back(this, id);
    }
    return out;
}

const std::string& Scene::EntityName(uint32_t id) const {
    static const std::string kEmpty;
    auto it = m_names.find(id);
    return it != m_names.end() ? it->second : kEmpty;
}

void Scene::Update(float dt) {
    if (m_updateFn) {
        for (auto e : Entities()) m_updateFn(e, dt);
    }
}

} // namespace Arc
