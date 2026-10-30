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

void Widget::setParent(Widget* p) { parent = p; }
Widget* Widget::getParent() const { return parent; }

glm::vec2 Widget::getWorldPosition() const
{
    if (parent)
        return parent->getWorldPosition() + position;
    return position;
}

bool Widget::containsPoint(const glm::vec2& worldPoint) const
{
    const glm::vec2 wp = getWorldPosition();
    return worldPoint.x >= wp.x &&
        worldPoint.x <= wp.x + size.x &&
        worldPoint.y >= wp.y &&
        worldPoint.y <= wp.y + size.y;
}

void Widget::addChild(std::unique_ptr<Widget> child)
{
    child->setParent(this);
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
        child->update(ctx, deltaTime);
}

void Widget::draw(UIContext& ctx)
{
    if (!visible) return;
    for (auto& child : children)
        child->draw(ctx);
}

bool Widget::onMouseMove(const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;

    hovered = containsPoint(worldMousePos);

    // Front-to-back
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        if ((*it)->onMouseMove(worldMousePos))
            return true;
    }
    return hovered;
}

bool Widget::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled) return false;

    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        if ((*it)->onMouseButton(button, action, worldMousePos))
            return true;
    }
    return false;
}