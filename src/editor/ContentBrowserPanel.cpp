#include "ContentBrowserPanel.hpp"
#include "ui/UIContext.hpp"
#include "ui/Font.hpp"
#include "engine/FileDialog.hpp"
#include "engine/GameObject.hpp"

#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <cctype>

ContentBrowserPanel::ContentBrowserPanel()
{
    setPosition({ 10.0f, 580.0f });
    setSize({ 1260.0f, 160.0f });
    setBackgroundColor({ 0.11f, 0.12f, 0.15f, 0.96f });

    engineFilesCheckbox.setLabel("Show engine files");
    engineFilesCheckbox.setChecked(false);
    engineFilesCheckbox.setPosition({ 14.0f, 8.0f });
    engineFilesCheckbox.setSize({ 200.0f, 20.0f });
    engineFilesCheckbox.setOnChanged([this](bool checked)
        {
            showEngineFiles = checked;
            rebuildEntries();
        });

    searchBox.setPlaceholder("Search assets...");
    searchBox.setSize({ 220.0f, 24.0f });
    searchBox.setOnChanged([this](const std::string& text)
        {
            searchFilter = text;
            rebuildEntries();
        });

    refresh();
}

void ContentBrowserPanel::setScene(Scene* s) { scene = s; }
void ContentBrowserPanel::setSelectedObjectId(std::uint64_t* id) { selectedObjectId = id; }

void ContentBrowserPanel::refresh()
{
    if (ProjectManager::current())
        currentDir = ProjectManager::assetsDir();
    else
        currentDir = fs::path{};
    rebuildEntries();
}

void ContentBrowserPanel::rebuildEntries()
{
    entries.clear();
    if (currentDir.empty() || !fs::exists(currentDir))
        return;

    auto matchesFilter = [this](const std::string& name) -> bool
        {
            if (searchFilter.empty())
                return true;

            std::string lowerName = name;
            std::string lowerFilter = searchFilter;
            for (char& ch : lowerName)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            for (char& ch : lowerFilter)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

            return lowerName.find(lowerFilter) != std::string::npos;
        };

    try
    {
        for (auto& e : fs::directory_iterator(currentDir))
        {
            Entry ent;
            ent.name = e.path().filename().string();
            ent.path = e.path();
            ent.kind = e.is_directory() ? EntryKind::Folder : EntryKind::File;

            if (!ent.name.empty() && ent.name[0] == '.')
                continue;

            if (!matchesFilter(ent.name))
                continue;

            entries.push_back(std::move(ent));
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[ContentBrowser] " << ex.what() << "\n";
    }

    if (showEngineFiles && ProjectManager::current())
    {
#ifdef SIMPLERENDERER_ASSET_DIR
        fs::path engineAssets = fs::path(SIMPLERENDERER_ASSET_DIR);
        if (fs::exists(engineAssets) && currentDir == ProjectManager::assetsDir())
        {
            Entry eng;
            eng.kind = EntryKind::Folder;
            eng.name = "[Engine] assets";
            eng.path = engineAssets;
            if (matchesFilter(eng.name))
                entries.insert(entries.begin(), eng);
        }
#endif
    }

    std::sort(entries.begin(), entries.end(),
        [](const Entry& a, const Entry& b)
        {
            if (a.kind != b.kind)
                return a.kind == EntryKind::Folder;
            return a.name < b.name;
        });
}

void ContentBrowserPanel::goUp()
{
    if (currentDir.empty()) return;
    fs::path parent = currentDir.parent_path();
    if (ProjectManager::current())
    {
        fs::path assets = ProjectManager::assetsDir();
        if (currentDir == assets && !showEngineFiles) return;
    }
    if (!parent.empty())
    {
        currentDir = parent;
        rebuildEntries();
    }
}

void ContentBrowserPanel::openEntry(int index)
{
    if (index < 0 || index >= static_cast<int>(entries.size())) return;
    const Entry& e = entries[index];

    if (e.kind == EntryKind::Folder)
    {
        currentDir = e.path;
        rebuildEntries();
        return;
    }

    std::string ext = e.path.extension().string();
    for (char& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (ext == ".obj" && scene && selectedObjectId && *selectedObjectId != 0)
    {
        GameObject* obj = scene->findObjectById(*selectedObjectId);
        if (obj)
        {
            obj->setMeshPath(e.path.string());
            std::cout << "[ContentBrowser] Assigned mesh -> " << obj->getName() << "\n";
            return;
        }
    }

    std::cout << "[ContentBrowser] Selected: " << e.path << "\n";
}

void ContentBrowserPanel::createFolder()
{
    if (!ProjectManager::current()) return;
    if (currentDir.empty()) currentDir = ProjectManager::assetsDir();

    int n = 1;
    fs::path folder;
    do {
        folder = currentDir / ("NewFolder" + std::to_string(n++));
    } while (fs::exists(folder));

    std::error_code ec;
    if (fs::create_directory(folder, ec))
        rebuildEntries();
}

void ContentBrowserPanel::importAsset()
{
    if (!ProjectManager::current()) return;

    auto path = FileDialog::openFile(
        "Import Asset",
        "Assets\0*.png;*.obj;*.fbx\0Images\0*.png;*.jpg\0Models\0*.obj;*.fbx\0All\0*.*\0");

    if (!path) return;

    fs::path src(*path);
    std::string ext = src.extension().string();
    for (char& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    fs::path destDir = currentDir.empty() ? ProjectManager::assetsDir() : currentDir;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
        destDir = ProjectManager::texturesDir();
    else if (ext == ".obj" || ext == ".fbx")
        destDir = ProjectManager::modelsDir();

    std::error_code ec;
    fs::create_directories(destDir, ec);
    fs::path dest = destDir / src.filename();
    fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
    if (!ec)
    {
        currentDir = destDir;
        rebuildEntries();
    }
}

void ContentBrowserPanel::update(UIContext& ctx, float deltaTime)
{
    Panel::update(ctx, deltaTime);
    engineFilesCheckbox.update(ctx, deltaTime);
}

void ContentBrowserPanel::draw(UIContext& ctx)
{
    if (!visible)
        return;

    hits.clear();
    Panel::draw(ctx);

    const glm::vec2 wp = getWorldPosition();

    Font::draw(ctx, wp + glm::vec2(220.0f, 12.0f),
        "Content Browser", glm::vec4(0.85f, 0.9f, 1.0f, 1.0f), 1.5f);

    std::string pathStr = currentDir.empty() ? "(no project)" : currentDir.string();
    if (pathStr.size() > 60)
        pathStr = "..." + pathStr.substr(pathStr.size() - 57);
    Font::draw(ctx, wp + glm::vec2(420.0f, 14.0f), pathStr, glm::vec4(0.55f), 1.1f);

    engineFilesCheckbox.setPosition(wp + glm::vec2(14.0f, 10.0f));
    engineFilesCheckbox.draw(ctx);

    searchBox.setPosition(wp + glm::vec2(14.0f, 36.0f));
    searchBox.setSize({ 240.0f, 24.0f });
    searchBox.draw(ctx);

    auto drawBtn = [&](float x, const char* label, Hit::Type type)
        {
            glm::vec2 p = wp + glm::vec2(x, 36.0f);
            float tw = Font::getTextWidth(label, 1.25f);
            glm::vec2 s = { std::max(100.0f, tw + 24.0f), 26.0f };
            ctx.drawRect(p, s, glm::vec4(0.22f, 0.28f, 0.38f, 1.0f));
            Font::draw(ctx, p + glm::vec2((s.x - tw) * 0.5f, 6.0f), label, glm::vec4(1.0f), 1.25f);
            hits.push_back({ type, -1, p, s });
        };

    drawBtn(270.0f, "Up", Hit::Type::Up);
    drawBtn(380.0f, "New Folder", Hit::Type::NewFolder);
    drawBtn(520.0f, "Import", Hit::Type::Import);

    float x = 14.0f;
    float y = 72.0f;
    const float minCellW = 140.0f;
    const float cellH = 56.0f;
    const float gap = 8.0f;
    const float textScale = 1.2f;

    for (int i = 0; i < static_cast<int>(entries.size()); ++i)
    {
        std::string label = entries[i].name;
        const float textW = Font::getTextWidth(label, textScale);
        const float cellW = std::max(minCellW, textW + 24.0f);

        if (x + cellW > size.x - 10.0f)
        {
            x = 14.0f;
            y += cellH + gap;
            if (y + cellH > size.y - 4.0f)
                break;
        }

        glm::vec2 p = wp + glm::vec2(x, y);
        glm::vec2 s = { cellW, cellH };

        glm::vec4 bg = (i == hoveredIndex)
            ? glm::vec4(0.25f, 0.32f, 0.45f, 1.0f)
            : glm::vec4(0.15f, 0.16f, 0.20f, 1.0f);

        if (entries[i].kind == EntryKind::Folder)
        {
            bg = (i == hoveredIndex)
                ? glm::vec4(0.28f, 0.35f, 0.28f, 1.0f)
                : glm::vec4(0.16f, 0.20f, 0.16f, 1.0f);
        }

        ctx.drawRect(p, s, bg);

        const float tx = p.x + (cellW - textW) * 0.5f;
        const float ty = p.y + (cellH - 14.0f) * 0.5f - 4.0f;
        Font::draw(ctx, { tx, ty }, label, glm::vec4(0.95f), textScale);

        glm::vec4 bar(0.35f, 0.35f, 0.40f, 1.0f);
        if (entries[i].kind == EntryKind::Folder)
        {
            bar = glm::vec4(1.0f, 0.55f, 0.15f, 1.0f);
        }
        else
        {
            std::string ext = entries[i].path.extension().string();
            for (char& c : ext)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (ext == ".obj" || ext == ".fbx")
                bar = glm::vec4(0.90f, 0.20f, 0.20f, 1.0f);
            else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg")
                bar = glm::vec4(0.20f, 0.85f, 0.90f, 1.0f);
        }
        ctx.drawRect(p + glm::vec2(0.0f, cellH - 4.0f), { cellW, 4.0f }, bar);

        hits.push_back({ Hit::Type::Entry, i, p, s });
        x += cellW + gap;
    }
}

bool ContentBrowserPanel::onMouseMove(const glm::vec2& worldMousePos)
{
    hoveredIndex = -1;
    for (const auto& h : hits)
    {
        if (h.type != Hit::Type::Entry) continue;
        bool inside =
            worldMousePos.x >= h.pos.x && worldMousePos.x <= h.pos.x + h.size.x &&
            worldMousePos.y >= h.pos.y && worldMousePos.y <= h.pos.y + h.size.y;
        if (inside) { hoveredIndex = h.index; break; }
    }
    engineFilesCheckbox.onMouseMove(worldMousePos);
    return Panel::onMouseMove(worldMousePos);
}

bool ContentBrowserPanel::onMouseButton(int button, int action, const glm::vec2& worldMousePos)
{
    if (!visible || !enabled)
        return false;

    if (searchBox.onMouseButton(button, action, worldMousePos))
        return true;

    if (engineFilesCheckbox.onMouseButton(button, action, worldMousePos))
        return true;

    if (Panel::onMouseButton(button, action, worldMousePos))
        return true;

    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return false;

    for (const auto& h : hits)
    {
        bool inside =
            worldMousePos.x >= h.pos.x && worldMousePos.x <= h.pos.x + h.size.x &&
            worldMousePos.y >= h.pos.y && worldMousePos.y <= h.pos.y + h.size.y;

        if (!inside)
            continue;

        switch (h.type)
        {
        case Hit::Type::Up: goUp(); return true;
        case Hit::Type::NewFolder: createFolder(); return true;
        case Hit::Type::Import: importAsset(); return true;
        case Hit::Type::Entry: openEntry(h.index); return true;
        }
    }
    return false;
}

void ContentBrowserPanel::onKey(int key, int action)
{
    searchBox.onKey(key, action);
}

void ContentBrowserPanel::onChar(unsigned int codepoint)
{
    searchBox.onChar(codepoint);
}