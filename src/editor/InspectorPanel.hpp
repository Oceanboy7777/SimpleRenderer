#pragma once

#include "ui/Panel.hpp"
#include "engine/Scene.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

class InspectorPanel : public Panel
{
public:
    InspectorPanel();

    void setScene(Scene* scene);
    void setSelectedId(std::uint64_t id);

    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    enum class FieldType
    {
        PositionX, PositionY, PositionZ,
        ScaleX, ScaleY, ScaleZ,
        ColorR, ColorG, ColorB
    };

    struct ButtonHit
    {
        FieldType field;
        float delta;          // +0.25 or -0.25 etc.
        glm::vec2 pos;
        glm::vec2 size;
    };

    Scene* scene = nullptr;
    std::uint64_t selectedId = 0;
    std::vector<ButtonHit> buttonHits;

    void applyChange(FieldType field, float delta);
    void drawFloatRow(UIContext& ctx, float& y, const std::string& label,
        float value, FieldType field, float step);
};