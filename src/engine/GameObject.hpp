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

enum class UIType
{
    None = 0,
    UIRoot,          // "New User Interface" entry in main hierarchy
    Text,
    TextBox,
    ScrollFrame,
    Frame,
    CheckBox,
    Button
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

    // ---- UI support ----
    bool isUI() const;                       // true for UIRoot or any UI element
    bool isUIRoot() const;
    UIType getUIType() const;
    void setUIType(UIType type);

    bool isVisible() const;
    void setVisible(bool v);

    int getZOrder() const;
    void setZOrder(int z);

    const std::string& getText() const;
    void setText(const std::string& t);

    // 2D rect used inside UI Designer (local to the designer canvas)
    glm::vec2 getUISize() const;
    void setUISize(const glm::vec2& s);

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

    // UI
    UIType uiType = UIType::None;
    bool visible = true;
    int zOrder = 0;
    std::string textContent;
    glm::vec2 uiSize{ 120.0f, 40.0f };
};
