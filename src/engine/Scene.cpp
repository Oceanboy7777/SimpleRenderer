#include "Scene.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <cctype>

Scene::Scene()
    : nextObjectId(1)
{
    // Empty scene on project/creation — no default objects.
}

bool Scene::isNameTaken(const std::string& name, std::uint64_t excludeId) const
{
    for (const auto& obj : objects)
    {
        if (obj->getId() == excludeId) continue;
        if (obj->getName() == name) return true;
    }
    return false;
}

std::string Scene::makeUniqueName(const std::string& base) const
{
    if (!isNameTaken(base))
        return base;

    int counter = 1;
    while (true)
    {
        std::string candidate = base + " " + std::to_string(counter);
        if (!isNameTaken(candidate))
            return candidate;
        ++counter;
    }
}

bool Scene::tryRename(std::uint64_t id, const std::string& newName)
{
    if (newName.empty()) return false;
    GameObject* obj = findObjectById(id);
    if (!obj) return false;
    if (isNameTaken(newName, id)) return false;
    obj->setName(newName);
    return true;
}

GameObject& Scene::createObject(
    const std::string& name,
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& color,
    float rotationSpeed,
    MeshType meshType,
    MaterialType materialType
)
{
    const std::uint64_t id = nextObjectId++;
    std::string unique = makeUniqueName(name);

    auto object = std::make_unique<GameObject>(
        id,
        unique,
        position,
        scale,
        color,
        rotationSpeed,
        meshType,
        materialType
    );

    GameObject& reference = *object;
    objects.push_back(std::move(object));
    return reference;
}

GameObject& Scene::createUIObject(
    const std::string& desiredName,
    UIType type,
    std::uint64_t parentId
)
{
    const std::uint64_t id = nextObjectId++;
    std::string unique = makeUniqueName(desiredName);

    // UI objects live at origin; their "transform" is used for 2D placement in designer
    auto object = std::make_unique<GameObject>(
        id,
        unique,
        glm::vec3(0.0f),
        glm::vec3(1.0f),
        glm::vec3(0.9f, 0.9f, 0.95f),
        0.0f,
        MeshType::Cube,
        MaterialType::Default
    );

    object->setUIType(type);
    object->setParentId(parentId);
    object->setVisible(true);
    object->setZOrder(0);

    if (type == UIType::Text || type == UIType::Button || type == UIType::TextBox)
        object->setText(unique);
    else
        object->setText("");

    // Default sizes per type
    switch (type)
    {
    case UIType::Text:        object->setUISize({ 160.0f, 28.0f }); break;
    case UIType::TextBox:     object->setUISize({ 180.0f, 32.0f }); break;
    case UIType::Button:      object->setUISize({ 140.0f, 36.0f }); break;
    case UIType::CheckBox:    object->setUISize({ 120.0f, 28.0f }); break;
    case UIType::Frame:       object->setUISize({ 220.0f, 140.0f }); break;
    case UIType::ScrollFrame: object->setUISize({ 240.0f, 160.0f }); break;
    case UIType::UIRoot:      object->setUISize({ 0.0f, 0.0f }); break;
    default: break;
    }

    GameObject& reference = *object;
    objects.push_back(std::move(object));
    return reference;
}

GameObject* Scene::findObjectById(std::uint64_t id)
{
    for (auto& obj : objects)
        if (obj->getId() == id) return obj.get();
    return nullptr;
}

const GameObject* Scene::findObjectById(std::uint64_t id) const
{
    for (const auto& obj : objects)
        if (obj->getId() == id) return obj.get();
    return nullptr;
}

bool Scene::removeObject(std::uint64_t id)
{
    auto it = std::find_if(objects.begin(), objects.end(),
        [id](const std::unique_ptr<GameObject>& o) { return o->getId() == id; });
    if (it == objects.end()) return false;
    objects.erase(it);
    return true;
}

bool Scene::setParent(std::uint64_t childId, std::uint64_t parentId)
{
    GameObject* child = findObjectById(childId);
    if (!child) return false;
    if (parentId != 0 && !findObjectById(parentId)) return false;
    child->setParentId(parentId);
    return true;
}

glm::mat4 Scene::getWorldMatrix(std::uint64_t id) const
{
    return getWorldMatrixRecursive(id, 0);
}

glm::mat4 Scene::getWorldMatrixRecursive(std::uint64_t id, int depth) const
{
    if (depth > 32) return glm::mat4(1.0f);
    const GameObject* obj = findObjectById(id);
    if (!obj) return glm::mat4(1.0f);
    glm::mat4 local = obj->getModelMatrix();
    if (obj->getParentId() == 0) return local;
    return getWorldMatrixRecursive(obj->getParentId(), depth + 1) * local;
}

void Scene::update(float deltaTime)
{
    for (auto& obj : objects)
        obj->update(deltaTime);
}

const std::vector<std::unique_ptr<GameObject>>& Scene::getObjects() const
{
    return objects;
}
