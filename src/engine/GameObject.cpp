
#include "GameObject.hpp"

#include <glm/glm.hpp>

GameObject::GameObject(
    std::uint64_t id,
    const std::string& name,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& color,
    float rotationSpeed,
    MeshType meshType,
    MaterialType materialType
)
    : id(id),
    name(name),
    transform(),
    color(color),
    rotationSpeed(rotationSpeed),
    meshType(meshType),
    materialType(materialType),
    parentId(0)
{
    transform.setPosition(position);
    transform.setScale(scale);
}

void GameObject::update(float deltaTime)
{
    transform.rotate(
        glm::vec3(0.0f, rotationSpeed * deltaTime, 0.0f)
    );
}

void GameObject::setColor(const glm::vec3& c)
{
    color = c;
}

std::uint64_t GameObject::getId() const
{
    return id;
}

const std::string& GameObject::getName() const
{
    return name;
}

void GameObject::setName(const std::string& newName)
{
    name = newName;
}

Transform& GameObject::getTransform()
{
    return transform;
}

const Transform& GameObject::getTransform() const
{
    return transform;
}

glm::mat4 GameObject::getModelMatrix() const
{
    return transform.getLocalMatrix();
}

glm::vec3 GameObject::getPosition() const
{
    return transform.getPosition();
}

glm::vec3 GameObject::getScale() const
{
    return transform.getScale();
}

glm::vec3 GameObject::getColor() const
{
    return color;
}

MeshType GameObject::getMeshType() const
{
    return meshType;
}

MaterialType GameObject::getMaterialType() const
{
    return materialType;
}

void GameObject::setMaterialType(MaterialType newMaterialType)
{
    materialType = newMaterialType;
}

std::uint64_t GameObject::getParentId() const
{
    return parentId;
}

void GameObject::setParentId(std::uint64_t newParentId)
{
    parentId = newParentId;
}
