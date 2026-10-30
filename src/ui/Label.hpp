#pragma once

#include "Widget.hpp"
#include <string>
#include <glm/glm.hpp>

class Label : public Widget
{
public:
    Label();

    void setText(const std::string& text);
    void setColor(const glm::vec4& color);
    void setScale(float scale);

    void draw(UIContext& ctx) override;

private:
    std::string text;
    glm::vec4 color{ 0.9f, 0.9f, 0.9f, 1.0f };
    float scale = 1.5f;
};