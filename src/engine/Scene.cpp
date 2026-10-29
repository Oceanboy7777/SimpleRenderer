
#include "Scene.hpp"

#include <algorithm>
#include <memory>
#include <utility>

Scene::Scene()
    : nextObjectId(1)
{
    // Ground plane with its own material.
    createObject(
        "Ground",
        glm::vec3(0.0f, -1.02f, 0.0f),
        glm::vec3(24.0f, 1.0f, 24.0f),
        glm::vec3(0.75f, 0.85f, 0.75f),
        0.0f,
        MeshType::Plane,
        MaterialType::Ground
    );

    // Central cube.
    createObject(
        "Central Cube",
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f),
        glm::vec3(0.9f, 0.2f, 0.2f),
        0.7f
    );

    // Left cube.
    createObject(
        "Left Cube",
        glm::vec3(-2.5f, 0.0f, 0.0f),
        glm::vec3(0.65f),
        glm::vec3(0.2f, 0.7f, 1.0f),
        -0.9f
    );

    // Right cube.
    createObject(
        "Right Cube",
        glm::vec3(2.5f, 0.0f, 0.0f),
        glm::vec3(0.65f),
        glm::vec3(0.3f, 1.0f, 0.4f),
        1.1f
    );

    // Floating cube behind the central cube.
    createObject(
        "Floating Cube",
        glm::vec3(0.0f, 2.0f, -2.0f),
        glm::vec3(0.45f),
        glm::vec3(0.9f, 0.7f, 0.2f),
        1.4f
    );

    // Imported OBJ model to verify the Milestone 7 model pipeline.
    createObject(
        "Imported Pyramid",
        glm::vec3(3.2f, 0.0f, -2.5f),
        glm::vec3(0.8f),
        glm::vec3(0.85f, 0.65f, 1.0f),
        0.35f,
        MeshType::ImportedModel
    );

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

    auto object = std::make_unique<GameObject>(
        id,
        name,
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

GameObject* Scene::findObjectById(std::uint64_t id)
{
    for (const auto& object : objects)
    {
        if (object->getId() == id)
        {
            return object.get();
        }
    }

    return nullptr;
}

const GameObject* Scene::findObjectById(
    std::uint64_t id
) const
{
    for (const auto& object : objects)
    {
        if (object->getId() == id)
        {
            return object.get();
        }
    }

    return nullptr;
}

bool Scene::removeObject(std::uint64_t id)
{
    const auto iterator = std::find_if(
        objects.begin(),
        objects.end(),
        [id](const std::unique_ptr<GameObject>& object)
        {
            return object->getId() == id;
        }
    );

    if (iterator == objects.end())
    {
        return false;
    }

    // Detach direct children before removing the parent.
    for (const auto& object : objects)
    {
        if (object->getParentId() == id)
        {
            object->setParentId(0);
        }
    }

    objects.erase(iterator);

    return true;
}

bool Scene::setParent(
    std::uint64_t childId,
    std::uint64_t parentId
)
{
    GameObject* child = findObjectById(childId);

    if (child == nullptr)
    {
        return false;
    }

    if (parentId == 0)
    {
        child->setParentId(0);
        return true;
    }

    GameObject* parent = findObjectById(parentId);

    if (parent == nullptr || childId == parentId)
    {
        return false;
    }

    std::uint64_t currentId = parentId;

    for (std::size_t depth = 0; depth <= objects.size(); ++depth)
    {
        if (currentId == childId)
        {
            return false;
        }

        const GameObject* current = findObjectById(currentId);

        if (current == nullptr)
        {
            return false;
        }

        currentId = current->getParentId();

        if (currentId == 0)
        {
            child->setParentId(parentId);
            return true;
        }
    }

    return false;
}

glm::mat4 Scene::getWorldMatrix(std::uint64_t id) const
{
    return getWorldMatrixRecursive(id, 0);
}

glm::mat4 Scene::getWorldMatrixRecursive(
    std::uint64_t id,
    int depth
) const
{
    if (depth > static_cast<int>(objects.size()))
    {
        return glm::mat4(1.0f);
    }

    const GameObject* object = findObjectById(id);

    if (object == nullptr)
    {
        return glm::mat4(1.0f);
    }

    const glm::mat4 localMatrix =
        object->getTransform().getLocalMatrix();

    const std::uint64_t parentId = object->getParentId();

    if (parentId == 0)
    {
        return localMatrix;
    }

    return getWorldMatrixRecursive(parentId, depth + 1)
        * localMatrix;
}

void Scene::update(float deltaTime)
{
    for (const auto& object : objects)
    {
        object->update(deltaTime);
    }
}

const std::vector<std::unique_ptr<GameObject>>&
Scene::getObjects() const
{
    return objects;
}
