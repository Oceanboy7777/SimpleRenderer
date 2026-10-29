#include "Widget.hpp"
#include "UIContext.hpp"

Widget::Widget() = default;

void Widget::setPosition(const glm::vec2& pos) { position = pos; }
void Widget::setSize(const glm::vec2& size) { this->size = size; }
void Widget::setVisible(bool v) { visible = v; }
void Widget::setEnabled(bool e) { enabled = e; }

glm::vec2 Widget::getPosition() const { return position; }
glm::vec2 Widget::getSize() const { return size; }
bool Widget::isVisible() const { return visible; }
bool Widget::isEnabled() const { return enabled; }

void Widget::addChild(std::unique_ptr<Widget> child)
{
    children.push_back(std::move(child));
}

const std::vector<std::unique_ptr<Widget>>& Widget::getChildren() const
{
    return children;
}

void Widget::update(UIContext& ctx, float deltaTime)
{
    if (!visible) return;

    for (auto& child : children)
    {
        child->update(ctx, deltaTime);
    }
}

void Widget::draw(UIContext& ctx)
{
    if (!visible) return;

    for (auto& child : children)
    {
        child->draw(ctx);
    }
}

bool Widget::onMouseMove(const glm::vec2& mousePos)
{
    if (!visible || !enabled) return false;

    const bool inside =
        mousePos.x >= position.x &&
        mousePos.x <= position.x + size.x &&
        mousePos.y >= position.y &&
        mousePos.y <= position.y + size.y;

    hovered = inside;

    // Children first (front-to-back)
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        if ((*it)->onMouseMove(mousePos))
            return true;
    }

    return inside;
}

bool Widget::onMouseButton(int button, int action, const glm::vec2& mousePos)
{
    if (!visible || !enabled) return false;

    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        if ((*it)->onMouseButton(button, action, mousePos))
            return true;
    }

    return false;
}