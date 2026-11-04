#include "SceneSerializer.hpp"
#include "GameObject.hpp"
#include "ProjectManager.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

std::string SceneSerializer::defaultScenePath()
{
    if (ProjectManager::current())
        return (ProjectManager::scenesDir() / "main.scene").string();

#ifdef SIMPLERENDERER_ASSET_DIR
    return std::string(SIMPLERENDERER_ASSET_DIR) + "/scenes/main.scene";
#else
    return "assets/scenes/main.scene";
#endif
}

bool SceneSerializer::save(const Scene& scene, const std::string& path)
{
    const std::string finalPath = path.empty() ? defaultScenePath() : path;

    try
    {
        fs::path p(finalPath);
        if (p.has_parent_path())
            fs::create_directories(p.parent_path());
    }
    catch (const std::exception& e)
    {
        std::cerr << "[SceneSerializer] create_directories failed: " << e.what() << "\n";
        return false;
    }

    std::ofstream file(finalPath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Failed to WRITE: " << finalPath << "\n";
        return false;
    }

    file << "# SimpleRenderer scene v2\n";

    for (const auto& obj : scene.getObjects())
    {
        const glm::vec3 p = obj->getPosition();
        const glm::vec3 s = obj->getScale();
        const glm::vec3 c = obj->getColor();
        const int mesh = static_cast<int>(obj->getMeshType());
        const int mat = static_cast<int>(obj->getMaterialType());
        const int ui = static_cast<int>(obj->getUIType());
        const int vis = obj->isVisible() ? 1 : 0;
        const int zo = obj->getZOrder();
        const glm::vec2 us = obj->getUISize();

        file << "object \"" << obj->getName() << "\" "
            << p.x << ' ' << p.y << ' ' << p.z << ' '
            << s.x << ' ' << s.y << ' ' << s.z << ' '
            << c.r << ' ' << c.g << ' ' << c.b << ' '
            << mesh << ' ' << mat
            << " \"" << obj->getMeshPath() << "\""
            << " " << ui << " " << vis << " " << zo
            << " " << us.x << " " << us.y
            << " " << obj->getParentId()
            << " \"" << obj->getText() << "\"\n";
    }

    file.flush();
    if (!file.good())
        return false;

    std::cout << "[SceneSerializer] Saved " << scene.getObjects().size()
        << " objects -> " << finalPath << "\n";
    return true;
}

bool SceneSerializer::load(Scene& scene, const std::string& path)
{
    const std::string finalPath = path.empty() ? defaultScenePath() : path;

    std::ifstream file(finalPath);
    if (!file.is_open())
    {
        std::cerr << "[SceneSerializer] Failed to READ: " << finalPath << "\n";
        return false;
    }

    {
        std::vector<std::uint64_t> ids;
        for (const auto& obj : scene.getObjects())
            ids.push_back(obj->getId());
        for (std::uint64_t id : ids)
            scene.removeObject(id);
    }

    int loaded = 0;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag != "object")
            continue;

        ss >> std::ws;
        std::string name;
        if (ss.peek() == '"')
        {
            ss.get();
            std::getline(ss, name, '"');
        }
        else
        {
            ss >> name;
        }

        float px, py, pz, sx, sy, sz, cr, cg, cb;
        int mesh = 0, mat = 0;
        if (!(ss >> px >> py >> pz >> sx >> sy >> sz >> cr >> cg >> cb >> mesh >> mat))
            continue;

        ss >> std::ws;
        std::string meshPath;
        if (ss.peek() == '"')
        {
            ss.get();
            std::getline(ss, meshPath, '"');
        }

        int ui = 0, vis = 1, zo = 0;
        float usx = 120.0f, usy = 40.0f;
        std::uint64_t parentId = 0;
        ss >> ui >> vis >> zo >> usx >> usy >> parentId;

        ss >> std::ws;
        std::string textContent;
        if (ss.peek() == '"')
        {
            ss.get();
            std::getline(ss, textContent, '"');
        }

        MeshType meshType = MeshType::Cube;
        if (mesh == static_cast<int>(MeshType::Plane))
            meshType = MeshType::Plane;
        else if (mesh == static_cast<int>(MeshType::ImportedModel))
            meshType = MeshType::ImportedModel;

        MaterialType matType = MaterialType::Default;
        if (mat == static_cast<int>(MaterialType::Ground))
            matType = MaterialType::Ground;

        UIType uiType = static_cast<UIType>(ui);

        GameObject* createdPtr = nullptr;
        if (uiType != UIType::None)
        {
            GameObject& created = scene.createUIObject(name, uiType, parentId);
            created.getTransform().setPosition(glm::vec3(px, py, pz));
            created.getTransform().setScale(glm::vec3(sx, sy, sz));
            created.setColor(glm::vec3(cr, cg, cb));
            created.setVisible(vis != 0);
            created.setZOrder(zo);
            created.setUISize({ usx, usy });
            created.setText(textContent);
            // Force exact name if unique system changed it
            scene.tryRename(created.getId(), name);
            createdPtr = &created;
        }
        else
        {
            GameObject& created = scene.createObject(
                name,
                glm::vec3(px, py, pz),
                glm::vec3(sx, sy, sz),
                glm::vec3(cr, cg, cb),
                0.0f,
                meshType,
                matType
            );
            if (!meshPath.empty())
                created.setMeshPath(meshPath);
            created.setVisible(vis != 0);
            createdPtr = &created;
        }

        (void)createdPtr;
        ++loaded;
    }

    std::cout << "[SceneSerializer] Loaded " << loaded
        << " objects <- " << finalPath << "\n";
    return true;
}
