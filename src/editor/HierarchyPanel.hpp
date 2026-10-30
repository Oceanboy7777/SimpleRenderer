#pragma once

#include "ui/Panel.hpp"
#include "engine/Scene.hpp"
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

class HierarchyPanel : public Panel
{
public:
    HierarchyPanel();

    void setScene(Scene* scene);
    void setSelectedId(std::uint64_t id);
    std::uint64_t getSelectedId() const;

    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseMove(const glm::vec2& worldMousePos) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    Scene* scene = nullptr;
    std::uint64_t selectedId = 0;
    std::uint64_t hoveredId = 0;

    struct ItemRect
    {
        std::uint64_t id = 0;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    struct ActionButton
    {
        enum class Type { CreateCube, Delete, SaveScene, LoadScene } type;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    std::vector<ItemRect> itemRects;
    std::vector<ActionButton> actionButtons;

    void createCube();
    void deleteSelected();

    void saveScene();
    void loadScene();
};