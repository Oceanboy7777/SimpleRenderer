#include "TextBox.hpp"
#include "UIContext.hpp"
#include "Font.hpp"

#include <GLFW/glfw3.h>

TextBox::TextBox()
{
    size = { 160.0f, 26.0f };
}

void TextBox::setText(const std::string& t)
{
    text = t;
}

const std::string& TextBox::getText() const
{
    return text;
}

void TextBox::setPlaceholder(const std::string& t)
{
    placeholder = t;
}

void TextBox::setFocused(bool f)
{
    focused = f;
}

bool TextBox::isFocused() const
{
    return focused;
}

void TextBox::setOnSubmit(std::function<void(const std::string&)> cb)
{
    onSubmit = std::move(cb);
}

void TextBox::setOnChanged(std::function<void(const std::string&)> cb)
{
    onChanged = std::move(cb);
}

void TextBox::draw(UIContext& ctx)
{
    if (!visible)
        return;

    const glm::vec2 wp = getWorldPosition();

    ctx.drawRect(wp, size, focused
        ? glm::vec4(0.16f, 0.18f, 0.24f, 1.0f)
        : glm::vec4(0.12f, 0.13f, 0.16f, 1.0f));

    ctx.drawRect(wp, { size.x, 1.0f }, glm::vec4(0.35f));
    ctx.drawRect(wp + glm::vec2(0.0f, size.y - 1.0f), { size.x, 1.0f }, glm::vec4(0.35f));
    ctx.drawRect(wp, { 1.0f, size.y }, glm::vec4(0.35f));
    ctx.drawRect(wp + glm::vec2(size.x - 1.0f, 0.0f), { 1.0f, size.y }, glm::vec4(0.35f));

    const float pad = 8.0f;
    const float textY = wp.y + (size.y - 12.0f) * 0.5f;

    if (!text.empty())
    {
        std::string drawText = text;
        if (focused)
            drawText += "_";
        Font::draw(ctx, wp + glm::vec2(pad, textY - wp.y), drawText, glm::vec4(0.95f), 1.25f);
    }
    else if (!placeholder.empty() && !focused)
    {
        Font::draw(ctx, wp + glm::vec2(pad, textY - wp.y), placeholder, glm::vec4(0.45f), 1.2f);
    }
    else if (focused)
    {
        Font::draw(ctx, wp + glm::vec2(pad, textY - wp.y), "_", glm::vec4(0.8f), 1.25f);
    }
}

bool TextBox::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        focused = containsPoint(worldMousePos);
        return focused;
    }

    return false;
}

void TextBox::onKey(int key, int action)
{
    if (!focused)
        return;

    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;

    if (key == GLFW_KEY_BACKSPACE && !text.empty())
    {
        text.pop_back();
        if (onChanged)
            onChanged(text);
    }
    else if (key == GLFW_KEY_ENTER)
    {
        if (onSubmit)
            onSubmit(text);
        focused = false;
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        focused = false;
    }
}

void TextBox::onChar(unsigned int codepoint)
{
    if (!focused)
        return;

    if (codepoint < 32 || codepoint > 126)
        return;

    if (text.size() >= 64)
        return;

    text.push_back(static_cast<char>(codepoint));
    if (onChanged)
        onChanged(text);
}