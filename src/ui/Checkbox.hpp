#pragma once

#include "Widget.hpp"
#include <functional>
#include <string>
#include <glm/glm.hpp>

class Checkbox : public Widget
{
public:
    Checkbox();

    void setLabel(const std::string& text);
    void setChecked(bool checked);
    bool isChecked() const;

    void setOnChanged(std::function<void(bool)> cb);

    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

private:
    std::string label;
    bool checked = false;
    std::function<void(bool)> onChanged;
};