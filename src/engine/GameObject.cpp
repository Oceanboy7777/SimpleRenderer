#include "GameObject.hpp"

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
    : id(id)
    , name(name)
    , transform()
    , color(color)
    , rotationSpeed(rotationSpeed)
    , meshType(meshType)
    , materialType(materialType)
    , parentId(0)
    , meshPath()
    , texturePath()
{
    transform.setPosition(position);
    transform.setScale(scale);
}

void GameObject::update(float deltaTime)
{
    transform.rotate(glm::vec3(0.0f, rotationSpeed * deltaTime, 0.0f));
}

void GameObject::setColor(const glm::vec3& c) { color = c; }
std::uint64_t GameObject::getId() const { return id; }
const std::string& GameObject::getName() const { return name; }
void GameObject::setName(const std::string& n) { name = n; }
Transform& GameObject::getTransform() { return transform; }
const Transform& GameObject::getTransform() const { return transform; }
glm::mat4 GameObject::getModelMatrix() const { return transform.getLocalMatrix(); }
glm::vec3 GameObject::getPosition() const { return transform.getPosition(); }
glm::vec3 GameObject::getScale() const { return transform.getScale(); }
glm::vec3 GameObject::getColor() const { return color; }
MeshType GameObject::getMeshType() const { return meshType; }
void GameObject::setMeshType(MeshType type) { meshType = type; }
MaterialType GameObject::getMaterialType() const { return materialType; }
void GameObject::setMaterialType(MaterialType t) { materialType = t; }
std::uint64_t GameObject::getParentId() const { return parentId; }
void GameObject::setParentId(std::uint64_t p) { parentId = p; }

const std::string& GameObject::getMeshPath() const { return meshPath; }

void GameObject::setMeshPath(const std::string& path)
{
    meshPath = path;
    if (!meshPath.empty())
        meshType = MeshType::ImportedModel;
}

void GameObject::clearMeshPath()
{
    meshPath.clear();
}

const std::string& GameObject::getTexturePath() const { return texturePath; }
void GameObject::setTexturePath(const std::string& path) { texturePath = path; }
void GameObject::clearTexturePath() { texturePath.clear(); }