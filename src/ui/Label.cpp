#include "Label.hpp"
#include "UIContext.hpp"
#include "Font.hpp"

Label::Label()
{
    size = { 100.0f, 20.0f };
}

void Label::setText(const std::string& t) { text = t; }
void Label::setColor(const glm::vec4& c) { color = c; }
void Label::setScale(float s) { scale = s; }

void Label::draw(UIContext& ctx)
{
    if (!visible || text.empty()) return;

    Font::draw(ctx, getWorldPosition(), text, color, scale);
    Widget::draw(ctx);
}