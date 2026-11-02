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
    void setColor(const glm::vec3& color);

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
    void setMeshType(MeshType type);

    MaterialType getMaterialType() const;
    void setMaterialType(MaterialType materialType);

    std::uint64_t getParentId() const;
    void setParentId(std::uint64_t parentId);

    const std::string& getMeshPath() const;
    void setMeshPath(const std::string& path);
    void clearMeshPath();

    const std::string& getTexturePath() const;
    void setTexturePath(const std::string& path);
    void clearTexturePath();

private:
    std::uint64_t id;
    std::string name;
    Transform transform;
    glm::vec3 color;
    float rotationSpeed;
    MeshType meshType;
    MaterialType materialType;
    std::uint64_t parentId;
    std::string meshPath;
    std::string texturePath;
};