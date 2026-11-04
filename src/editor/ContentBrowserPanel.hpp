#pragma once

#include "ui/Panel.hpp"
#include "ui/Checkbox.hpp"
#include "ui/TextBox.hpp"
#include "ui/ScrollFrame.hpp"

#include "engine/ProjectManager.hpp"
#include "engine/Scene.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace fs = std::filesystem;

class ContentBrowserPanel : public Panel
{
public:
    ContentBrowserPanel();

    void setScene(Scene* scene);
    void setSelectedObjectId(std::uint64_t* selectedId);

    void refresh();
    void update(UIContext& ctx, float deltaTime) override;
    void draw(UIContext& ctx) override;
    bool onMouseButton(int button, int action, const glm::vec2& worldMousePos) override;
    bool onMouseMove(const glm::vec2& worldMousePos) override;

    void onKey(int key, int action);
    void onChar(unsigned int codepoint);
    void onScroll(float yOffset);

private:
    enum class EntryKind { Folder, File };

    struct Entry
    {
        EntryKind kind = EntryKind::File;
        std::string name;
        fs::path path;
    };

    struct Hit
    {
        enum class Type { Entry, Up, NewFolder, Import } type;
        int index = -1;
        glm::vec2 pos{ 0.0f };
        glm::vec2 size{ 0.0f };
    };

    Scene* scene = nullptr;
    std::uint64_t* selectedObjectId = nullptr;

    TextBox searchBox;
    std::string searchFilter;

    fs::path currentDir;
    std::vector<Entry> entries;
    std::vector<Hit> hits;
    int hoveredIndex = -1;

    bool showEngineFiles = false;
    Checkbox engineFilesCheckbox;

    void rebuildEntries();
    void goUp();
    void openEntry(int index);
    void createFolder();
    void importAsset();

    ScrollFrame gridScroll;
    float gridContentH = 0.0f;
};