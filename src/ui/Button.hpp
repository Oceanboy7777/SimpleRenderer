#pragma once

#include "Widget.hpp"
#include <functional>
#include <string>

class Button : public Widget
{
public:
    Button();

    void setLabel(const std::string& text);
    void setOnClick(std::function<void()> callback);

    void setNormalColor(const glm::vec4& color);
    void setHoveredColor(const glm::vec4& color);
    void setPressedColor(const glm::vec4& color);

    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& mousePos) override;

private:
    std::string label;
    std::function<void()> onClick;

    glm::vec4 normalColor{ 0.25f, 0.45f, 0.75f, 1.0f };
    glm::vec4 hoveredColor{ 0.35f, 0.55f, 0.90f, 1.0f };
    glm::vec4 pressedColor{ 0.15f, 0.30f, 0.55f, 1.0f };

    bool pressed = false;
};