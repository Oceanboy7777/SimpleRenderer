#pragma once

#include "Widget.hpp"

#include <functional>
#include <string>
#include <glm/glm.hpp>

class TextBox : public Widget
{
public:
    TextBox();

    void setText(const std::string& text);
    const std::string& getText() const;

    void setPlaceholder(const std::string& text);
    void setFocused(bool focused);
    bool isFocused() const;

    void setOnSubmit(std::function<void(const std::string&)> cb);
    void setOnChanged(std::function<void(const std::string&)> cb);

    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;

    void onKey(int key, int action);
    void onChar(unsigned int codepoint);

private:
    std::string text;
    std::string placeholder;
    bool focused = false;
    std::function<void(const std::string&)> onSubmit;
    std::function<void(const std::string&)> onChanged;
};