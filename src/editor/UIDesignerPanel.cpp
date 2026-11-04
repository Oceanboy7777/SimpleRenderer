#include "UIDesignerPanel.hpp"
#include "ContentBrowserPanel.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace {
    constexpr float kRowH = 28.0f;
    constexpr float kTopBarH = 36.0f;
    constexpr float kLeftW = 260.0f;
    constexpr float kRightW = 280.0f;
    constexpr float kBottomH = 180.0f; // content browser
}

UIDesignerPanel::UIDesignerPanel()
{
    setPosition({ 0.0f, 0.0f });
    setSize({ 1280.0f, 720.0f });
    setBackgroundColor({ 0.10f, 0.11f, 0.14f, 1.0f });
    setVisible(false); // IMPORTANT: hidden until opened

    nameBox.setPlaceholder("Name");
    zOrderBox.setPlaceholder("Z");
    textBox.setPlaceholder("Text");
    colRBox.setPlaceholder("R");
    colGBox.setPlaceholder("G");
    colBBox.setPlaceholder("B");

    nameBox.setSize({ 200.0f, 24.0f });
    zOrderBox.setSize({ 80.0f, 24.0f });
    textBox.setSize({ 200.0f, 24.0f });
    colRBox.setSize({ 50.0f, 24.0f });
    colGBox.setSize({ 50.0f, 24.0f });
    colBBox.setSize({ 50.0f, 24.0f });
}

void UIDesignerPanel::setScene(Scene* s) { scene = s; }
void UIDesignerPanel::setUIRootId(std::uint64_t id)
{
    uiRootId = id;
    selectedElementId = 0;
    createMenuOpen = false;
    dragging = false;
}
std::uint64_t UIDesignerPanel::getUIRootId() const { return uiRootId; }
void UIDesignerPanel::setOnClose(std::function<void()> cb) { onClose = std::move(cb); }
void UIDesignerPanel::setOnSave(std::function<void()> cb) { onSave = std::move(cb); }
void UIDesignerPanel::setContentBrowser(ContentBrowserPanel* browser) { contentBrowser = browser; }

bool UIDesignerPanel::hitTest(const glm::vec2& p, const glm::vec2& pos, const glm::vec2& size) const
{
    return p.x >= pos.x && p.x <= pos.x + size.x &&
        p.y >= pos.y && p.y <= pos.y + size.y;
}

void UIDesignerPanel::createElement(UIType type, const char* baseName)
{
    if (!scene || uiRootId == 0) return;

    GameObject& el = scene->createUIObject(baseName, type, uiRootId);
    // place near centre of canvas
    float cx = std::max(40.0f, canvasSize.x * 0.5f - 60.0f);
    float cy = std::max(40.0f, canvasSize.y * 0.5f - 20.0f);
    el.getTransform().setPosition(glm::vec3(cx, cy, 0.0f));
    selectedElementId = el.getId();
    createMenuOpen = false;
    syncInspectorFromSelection();
    std::cout << "UI Designer: created " << baseName << " id=" << selectedElementId << "\n";
}

void UIDesignerPanel::syncInspectorFromSelection()
{
    if (!scene || selectedElementId == 0) return;
    GameObject* obj = scene->findObjectById(selectedElementId);
    if (!obj) return;
    nameBox.setText(obj->getName());
    zOrderBox.setText(std::to_string(obj->getZOrder()));
    textBox.setText(obj->getText());
    const glm::vec3 c = obj->getColor();
    colRBox.setText(std::to_string(static_cast<int>(std::round(c.r * 255.0f))));
    colGBox.setText(std::to_string(static_cast<int>(std::round(c.g * 255.0f))));
    colBBox.setText(std::to_string(static_cast<int>(std::round(c.b * 255.0f))));
}

void UIDesignerPanel::applyInspector()
{
    if (!scene || selectedElementId == 0) return;
    GameObject* obj = scene->findObjectById(selectedElementId);
    if (!obj) return;

    std::string newName = nameBox.getText();
    if (!newName.empty() && newName != obj->getName())
    {
        if (!scene->tryRename(selectedElementId, newName))
            std::cout << "UI Designer: name already taken\n";
    }

    try { obj->setZOrder(std::stoi(zOrderBox.getText())); }
    catch (...) {}
    obj->setText(textBox.getText());

    auto parse255 = [](const std::string& s) -> float {
        try { return std::clamp(std::stof(s) / 255.0f, 0.0f, 1.0f); }
        catch (...) { return 1.0f; }
        };
    obj->setColor({ parse255(colRBox.getText()), parse255(colGBox.getText()), parse255(colBBox.getText()) });
}

void UIDesignerPanel::update(UIContext& ctx, float deltaTime)
{
    if (!visible) return;
    Panel::update(ctx, deltaTime);
    hierarchyScroll.update(ctx, deltaTime);
    nameBox.update(ctx, deltaTime);
    zOrderBox.update(ctx, deltaTime);
    textBox.update(ctx, deltaTime);
    colRBox.update(ctx, deltaTime);
    colGBox.update(ctx, deltaTime);
    colBBox.update(ctx, deltaTime);
}

void UIDesignerPanel::drawElementOnCanvas(UIContext& ctx, const GameObject& obj)
{
    glm::vec2 pos = canvasPos + glm::vec2(obj.getPosition().x, obj.getPosition().y);
    glm::vec2 size = obj.getUISize();
    const bool sel = (obj.getId() == selectedElementId);
    const bool hov = (obj.getId() == hoveredElementId);

    glm::vec4 col = glm::vec4(obj.getColor(), sel ? 0.95f : (hov ? 0.85f : 0.75f));
    if (obj.getUIType() == UIType::Frame || obj.getUIType() == UIType::ScrollFrame)
        col.a = 0.50f;

    ctx.drawRect(pos, size, col);

    // Thin white border on edges only when selected (not a full overlay)
    if (sel)
    {
        const float b = 2.0f;
        const glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
        ctx.drawRect(pos, { size.x, b }, white);
        ctx.drawRect(pos + glm::vec2(0.0f, size.y - b), { size.x, b }, white);
        ctx.drawRect(pos, { b, size.y }, white);
        ctx.drawRect(pos + glm::vec2(size.x - b, 0.0f), { b, size.y }, white);
    }

    std::string label;
    switch (obj.getUIType())
    {
    case UIType::Text:        label = obj.getText().empty() ? "Text" : obj.getText(); break;
    case UIType::TextBox:     label = obj.getText().empty() ? "TextBox" : obj.getText(); break;
    case UIType::Button:      label = obj.getText().empty() ? "Button" : obj.getText(); break;
    case UIType::CheckBox:    label = "[ ] " + (obj.getText().empty() ? "CheckBox" : obj.getText()); break;
    case UIType::Frame:       label = "Frame"; break;
    case UIType::ScrollFrame: label = "ScrollFrame"; break;
    default: label = obj.getName(); break;
    }
    Font::draw(ctx, pos + glm::vec2(8.0f, size.y * 0.5f - 6.0f), label, glm::vec4(1.0f), 1.15f);
}

void UIDesignerPanel::draw(UIContext& ctx)
{
    if (!visible) return;

    hits.clear();
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();
    const glm::vec2 sz = getSize();

    // ---- Top bar ----
    ctx.drawRect(wp, { sz.x, kTopBarH }, glm::vec4(0.14f, 0.15f, 0.19f, 1.0f));
    Font::draw(ctx, wp + glm::vec2(16.0f, 10.0f), "UI Designer", glm::vec4(0.95f), 1.4f);

    glm::vec2 savePos = { wp.x + sz.x - 190.0f, wp.y + 6.0f };
    glm::vec2 saveSize = { 84.0f, 24.0f };
    ctx.drawRect(savePos, saveSize, glm::vec4(0.20f, 0.45f, 0.30f, 1.0f));
    Font::draw(ctx, savePos + glm::vec2(24.0f, 5.0f), "Save", glm::vec4(1.0f), 1.2f);
    hits.push_back({ Hit::Type::Save, savePos, saveSize, 0 });

    closePos = { wp.x + sz.x - 96.0f, wp.y + 6.0f };
    closeSize = { 84.0f, 24.0f };
    ctx.drawRect(closePos, closeSize, glm::vec4(0.60f, 0.22f, 0.22f, 1.0f));
    Font::draw(ctx, closePos + glm::vec2(22.0f, 5.0f), "Close", glm::vec4(1.0f), 1.2f);
    hits.push_back({ Hit::Type::Close, closePos, closeSize, 0 });

    // ---- Left hierarchy ----
    glm::vec2 hierPos = { wp.x + 8.0f, wp.y + kTopBarH + 8.0f };
    glm::vec2 hierSize = { kLeftW - 16.0f, sz.y - kTopBarH - kBottomH - 16.0f };
    ctx.drawRect(hierPos, hierSize, glm::vec4(0.12f, 0.13f, 0.16f, 0.98f));
    Font::draw(ctx, hierPos + glm::vec2(10.0f, 8.0f), "Interface", glm::vec4(0.85f), 1.3f);

    plusPos = { hierPos.x + hierSize.x - 34.0f, hierPos.y + 6.0f };
    plusSize = { 26.0f, 22.0f };
    ctx.drawRect(plusPos, plusSize, glm::vec4(0.20f, 0.45f, 0.30f, 1.0f));
    Font::draw(ctx, plusPos + glm::vec2(7.0f, 3.0f), "+", glm::vec4(1.0f), 1.4f);
    hits.push_back({ Hit::Type::AddElement, plusPos, plusSize, 0 });

    hierarchyScroll.setPosition(hierPos + glm::vec2(4.0f, 36.0f));
    hierarchyScroll.setSize({ hierSize.x - 8.0f, hierSize.y - 44.0f });

    float localY = 4.0f;
    if (scene)
    {
        for (const auto& op : scene->getObjects())
        {
            const GameObject& o = *op;
            if (o.getParentId() != uiRootId || !o.isUI() || o.isUIRoot())
                continue;

            const float worldY = hierarchyScroll.contentToWorldY(localY);
            glm::vec2 rowPos = { hierarchyScroll.getWorldPosition().x + 4.0f, worldY };
            glm::vec2 rowSize = { hierarchyScroll.getSize().x - 8.0f, kRowH };

            const bool sel = (o.getId() == selectedElementId);
            ctx.drawRect(rowPos, rowSize,
                sel ? glm::vec4(0.22f, 0.38f, 0.72f, 0.95f)
                : glm::vec4(0.16f, 0.17f, 0.21f, 0.9f));
            Font::draw(ctx, rowPos + glm::vec2(8.0f, 6.0f), o.getName(), glm::vec4(0.95f), 1.15f);
            hits.push_back({ Hit::Type::SelectElement, rowPos, rowSize, o.getId() });
            localY += kRowH + 3.0f;
        }
    }
    hierarchyContentH = localY + 8.0f;
    hierarchyScroll.setContentHeight(hierarchyContentH);
    hierarchyScroll.draw(ctx);

    // ---- Create dropdown ----
    if (createMenuOpen)
    {
        menuPos = plusPos + glm::vec2(0.0f, 28.0f);
        menuSize = { 160.0f, 196.0f };
        ctx.drawRect(menuPos, menuSize, glm::vec4(0.16f, 0.17f, 0.22f, 0.98f));

        struct Opt { const char* label; Hit::Type type; };
        const Opt opts[] = {
            { "Text",         Hit::Type::CreateText },
            { "Text Box",     Hit::Type::CreateTextBox },
            { "Scroll Frame", Hit::Type::CreateScrollFrame },
            { "Frame",        Hit::Type::CreateFrame },
            { "Check Box",    Hit::Type::CreateCheckBox },
            { "Button",       Hit::Type::CreateButton },
        };
        float my = 8.0f;
        for (const auto& o : opts)
        {
            glm::vec2 p = menuPos + glm::vec2(8.0f, my);
            glm::vec2 s = { 144.0f, 26.0f };
            ctx.drawRect(p, s, glm::vec4(0.22f, 0.25f, 0.32f, 1.0f));
            Font::draw(ctx, p + glm::vec2(10.0f, 6.0f), o.label, glm::vec4(0.95f), 1.15f);
            hits.push_back({ o.type, p, s, 0 });
            my += 30.0f;
        }
    }

    // ---- Canvas ----
    canvasPos = { wp.x + kLeftW, wp.y + kTopBarH + 4.0f };
    canvasSize = { sz.x - kLeftW - kRightW, sz.y - kTopBarH - kBottomH - 8.0f };
    if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
    if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

    ctx.drawRect(canvasPos, canvasSize, glm::vec4(0.08f, 0.09f, 0.11f, 1.0f));
    for (float x = 0; x < canvasSize.x; x += 40.0f)
        ctx.drawRect(canvasPos + glm::vec2(x, 0), { 1.0f, canvasSize.y }, glm::vec4(0.12f, 0.13f, 0.15f, 0.5f));
    for (float y = 0; y < canvasSize.y; y += 40.0f)
        ctx.drawRect(canvasPos + glm::vec2(0, y), { canvasSize.x, 1.0f }, glm::vec4(0.12f, 0.13f, 0.15f, 0.5f));

    if (scene)
    {
        std::vector<const GameObject*> els;
        for (const auto& op : scene->getObjects())
        {
            if (op->getParentId() == uiRootId && op->isUI() && !op->isUIRoot())
                els.push_back(op.get());
        }
        std::sort(els.begin(), els.end(), [](const GameObject* a, const GameObject* b) {
            return a->getZOrder() < b->getZOrder();
            });
        for (const GameObject* o : els)
            drawElementOnCanvas(ctx, *o);
    }

    // ---- Right inspector ----
    glm::vec2 inspPos = { wp.x + sz.x - kRightW + 8.0f, wp.y + kTopBarH + 8.0f };
    glm::vec2 inspSize = { kRightW - 16.0f, sz.y - kTopBarH - kBottomH - 16.0f };
    ctx.drawRect(inspPos, inspSize, glm::vec4(0.12f, 0.13f, 0.16f, 0.98f));
    Font::draw(ctx, inspPos + glm::vec2(10.0f, 8.0f), "Inspector", glm::vec4(0.85f), 1.3f);

    float iy = inspPos.y + 40.0f;
    if (selectedElementId != 0)
    {
        Font::draw(ctx, { inspPos.x + 10.0f, iy }, "Name", glm::vec4(0.7f), 1.1f);
        iy += 18.0f;
        nameBox.setPosition({ inspPos.x + 10.0f, iy });
        nameBox.setSize({ inspSize.x - 20.0f, 24.0f });
        nameBox.draw(ctx);
        iy += 32.0f;

        Font::draw(ctx, { inspPos.x + 10.0f, iy }, "Z-Order", glm::vec4(0.7f), 1.1f);
        iy += 18.0f;
        zOrderBox.setPosition({ inspPos.x + 10.0f, iy });
        zOrderBox.draw(ctx);
        iy += 32.0f;

        Font::draw(ctx, { inspPos.x + 10.0f, iy }, "Text", glm::vec4(0.7f), 1.1f);
        iy += 18.0f;
        textBox.setPosition({ inspPos.x + 10.0f, iy });
        textBox.setSize({ inspSize.x - 20.0f, 24.0f });
        textBox.draw(ctx);
        iy += 32.0f;

        Font::draw(ctx, { inspPos.x + 10.0f, iy }, "Colour (0-255)", glm::vec4(0.7f), 1.1f);
        iy += 18.0f;
        colRBox.setPosition({ inspPos.x + 10.0f, iy });
        colGBox.setPosition({ inspPos.x + 70.0f, iy });
        colBBox.setPosition({ inspPos.x + 130.0f, iy });
        colRBox.draw(ctx);
        colGBox.draw(ctx);
        colBBox.draw(ctx);
    }
    else
    {
        Font::draw(ctx, { inspPos.x + 10.0f, iy }, "Select an element", glm::vec4(0.5f), 1.15f);
    }

    // Content browser strip at bottom (positioned by main; we reserve space via kBottomH)
    if (contentBrowser)
    {
        contentBrowser->setVisible(true);
        contentBrowser->setPosition({ wp.x, wp.y + sz.y - kBottomH });
        contentBrowser->setSize({ sz.x, kBottomH });
    }

    // ---- Create dropdown ALWAYS last so it is never clipped under the grid ----
    if (createMenuOpen)
    {
        // Clear previous create hits of type Create* then re-add (hits already has them from earlier draw)
        // Redraw menu on top
        menuPos = plusPos + glm::vec2(0.0f, 28.0f);
        menuSize = { 160.0f, 196.0f };
        ctx.drawRect(menuPos, menuSize, glm::vec4(0.16f, 0.17f, 0.22f, 1.0f));
        struct Opt { const char* label; Hit::Type type; };
        const Opt opts[] = {
            { "Text", Hit::Type::CreateText },
            { "Text Box", Hit::Type::CreateTextBox },
            { "Scroll Frame", Hit::Type::CreateScrollFrame },
            { "Frame", Hit::Type::CreateFrame },
            { "Check Box", Hit::Type::CreateCheckBox },
            { "Button", Hit::Type::CreateButton },
        };
        float my = 8.0f;
        for (const auto& o : opts)
        {
            glm::vec2 p = menuPos + glm::vec2(8.0f, my);
            glm::vec2 s = { 144.0f, 26.0f };
            ctx.drawRect(p, s, glm::vec4(0.22f, 0.25f, 0.32f, 1.0f));
            Font::draw(ctx, p + glm::vec2(10.0f, 6.0f), o.label, glm::vec4(0.95f), 1.15f);
            my += 30.0f;
        }
    }
}

bool UIDesignerPanel::onMouseMove(const glm::vec2& worldMousePos)
{
    if (!visible) return false;

    hoveredElementId = 0;

    if (dragging && scene && dragId != 0)
    {
        GameObject* obj = scene->findObjectById(dragId);
        if (obj)
        {
            glm::vec2 local = worldMousePos - canvasPos - dragOffset;
            obj->getTransform().setPosition(glm::vec3(local.x, local.y, 0.0f));
        }
        return true;
    }

    if (scene)
    {
        for (const auto& op : scene->getObjects())
        {
            if (op->getParentId() != uiRootId || !op->isUI() || op->isUIRoot()) continue;
            glm::vec2 p = canvasPos + glm::vec2(op->getPosition().x, op->getPosition().y);
            glm::vec2 s = op->getUISize();
            if (hitTest(worldMousePos, p, s))
            {
                hoveredElementId = op->getId();
                break;
            }
        }
    }

    hierarchyScroll.onMouseMove(worldMousePos);
    return containsPoint(worldMousePos);
}

bool UIDesignerPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        if (dragging)
        {
            dragging = false;
            dragId = 0;
            applyInspector();
            return true;
        }
    }

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    // Inspector text fields first
    if (nameBox.onMouseButton(button, action, worldMousePos)) return true;
    if (zOrderBox.onMouseButton(button, action, worldMousePos)) return true;
    if (textBox.onMouseButton(button, action, worldMousePos)) return true;
    if (colRBox.onMouseButton(button, action, worldMousePos)) return true;
    if (colGBox.onMouseButton(button, action, worldMousePos)) return true;
    if (colBBox.onMouseButton(button, action, worldMousePos)) return true;

    // Process hits (built during last draw)
    for (const auto& h : hits)
    {
        if (!hitTest(worldMousePos, h.pos, h.size))
            continue;

        switch (h.type)
        {
        case Hit::Type::Close:
            createMenuOpen = false;
            if (onClose) onClose();
            return true;

        case Hit::Type::AddElement:
            createMenuOpen = !createMenuOpen;
            return true;

        case Hit::Type::CreateText:
            createElement(UIType::Text, "Text");
            return true;
        case Hit::Type::CreateTextBox:
            createElement(UIType::TextBox, "TextBox");
            return true;
        case Hit::Type::CreateScrollFrame:
            createElement(UIType::ScrollFrame, "ScrollFrame");
            return true;
        case Hit::Type::CreateFrame:
            createElement(UIType::Frame, "Frame");
            return true;
        case Hit::Type::CreateCheckBox:
            createElement(UIType::CheckBox, "CheckBox");
            return true;
        case Hit::Type::CreateButton:
            createElement(UIType::Button, "Button");
            return true;

        case Hit::Type::SelectElement:
            selectedElementId = h.id;
            syncInspectorFromSelection();
            createMenuOpen = false;
            return true;
        }
    }

    // Canvas element drag / select
    if (hoveredElementId != 0 && scene)
    {
        selectedElementId = hoveredElementId;
        syncInspectorFromSelection();
        GameObject* obj = scene->findObjectById(hoveredElementId);
        if (obj)
        {
            dragging = true;
            dragId = hoveredElementId;
            dragOffset = worldMousePos - canvasPos - glm::vec2(obj->getPosition().x, obj->getPosition().y);
        }
        createMenuOpen = false;
        return true;
    }

    // Click empty canvas closes menu
    if (hitTest(worldMousePos, canvasPos, canvasSize))
    {
        createMenuOpen = false;
        return true;
    }

    // Consume click if inside panel
    if (containsPoint(worldMousePos))
    {
        createMenuOpen = false;
        return true;
    }
    return false;
}

void UIDesignerPanel::onKey(int key, int action)
{
    if (!visible) return;
    nameBox.onKey(key, action);
    zOrderBox.onKey(key, action);
    textBox.onKey(key, action);
    colRBox.onKey(key, action);
    colGBox.onKey(key, action);
    colBBox.onKey(key, action);
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
        applyInspector();
}

void UIDesignerPanel::onChar(unsigned int codepoint)
{
    if (!visible) return;
    nameBox.onChar(codepoint);
    zOrderBox.onChar(codepoint);
    textBox.onChar(codepoint);
    colRBox.onChar(codepoint);
    colGBox.onChar(codepoint);
    colBBox.onChar(codepoint);
    applyInspector();
}

void UIDesignerPanel::onScroll(float yOffset)
{
    if (!visible) return;
    hierarchyScroll.onScroll(yOffset);
}
