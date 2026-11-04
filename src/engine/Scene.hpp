#pragma once

#include "GameObject.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Scene
{
public:
    Scene();

    GameObject& createObject(
        const std::string& name,
        const glm::vec3& position,
        const glm::vec3& scale,
        const glm::vec3& color,
        float rotationSpeed = 0.0f,
        MeshType meshType = MeshType::Cube,
        MaterialType materialType = MaterialType::Default
    );

    // Creates a UI root ("New User Interface") or UI element. Name is forced unique.
    GameObject& createUIObject(
        const std::string& desiredName,
        UIType type,
        std::uint64_t parentId = 0
    );

    GameObject* findObjectById(std::uint64_t id);
    const GameObject* findObjectById(std::uint64_t id) const;

    bool removeObject(std::uint64_t id);

    bool setParent(
        std::uint64_t childId,
        std::uint64_t parentId
    );

    glm::mat4 getWorldMatrix(std::uint64_t id) const;

    void update(float deltaTime);

    const std::vector<std::unique_ptr<GameObject>>&
        getObjects() const;

    // Unique name helpers (apply to ALL game objects)
    bool isNameTaken(const std::string& name, std::uint64_t excludeId = 0) const;
    std::string makeUniqueName(const std::string& base) const;

    // Returns true if rename succeeded (name was free or already owned by this id)
    bool tryRename(std::uint64_t id, const std::string& newName);

private:
    glm::mat4 getWorldMatrixRecursive(
        std::uint64_t id,
        int depth
    ) const;

    std::vector<std::unique_ptr<GameObject>> objects;

    std::uint64_t nextObjectId;
};
