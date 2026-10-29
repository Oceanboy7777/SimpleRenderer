
#pragma once

#include "Transform.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>

enum class MeshType
{
    Cube,
    Plane,
    ImportedModel
};

enum class MaterialType
{
    Default,
    Ground
};

class GameObject
{
public:
    GameObject(
        std::uint64_t id,
        const std::string& name,
        const glm::vec3& position,
        const glm::vec3& scale,
        const glm::vec3& color,
        float rotationSpeed = 0.0f,
        MeshType meshType = MeshType::Cube,
        MaterialType materialType = MaterialType::Default
    );

    void update(float deltaTime);

    std::uint64_t getId() const;
    const std::string& getName() const;

    void setName(const std::string& name);

    Transform& getTransform();
    const Transform& getTransform() const;

    glm::mat4 getModelMatrix() const;

    glm::vec3 getPosition() const;
    glm::vec3 getScale() const;
    glm::vec3 getColor() const;

    MeshType getMeshType() const;

    MaterialType getMaterialType() const;
    void setMaterialType(MaterialType materialType);

    std::uint64_t getParentId() const;
    void setParentId(std::uint64_t parentId);

private:
    std::uint64_t id;
    std::string name;

    Transform transform;

    glm::vec3 color;
    float rotationSpeed;

    MeshType meshType;
    MaterialType materialType;

    std::uint64_t parentId;
};
