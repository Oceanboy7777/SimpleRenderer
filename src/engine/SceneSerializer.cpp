#include "SceneSerializer.hpp"
#include "GameObject.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

std::string SceneSerializer::defaultScenePath()
{
#ifdef SIMPLERENDERER_ASSET_DIR
    // Absolute path baked in at compile time — does not depend on cwd
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
        std::cerr << "[SceneSerializer] cwd = " << fs::current_path().string() << "\n";
        return false;
    }

    file << "# SimpleRenderer scene v1\n";

    for (const auto& obj : scene.getObjects())
    {
        const glm::vec3 p = obj->getPosition();
        const glm::vec3 s = obj->getScale();
        const glm::vec3 c = obj->getColor();

        const int mesh = static_cast<int>(obj->getMeshType());
        const int mat = static_cast<int>(obj->getMaterialType());

        file << "object \"" << obj->getName() << "\" "
            << p.x << ' ' << p.y << ' ' << p.z << ' '
            << s.x << ' ' << s.y << ' ' << s.z << ' '
            << c.r << ' ' << c.g << ' ' << c.b << ' '
            << mesh << ' ' << mat << '\n';
    }

    file.flush();
    if (!file.good())
    {
        std::cerr << "[SceneSerializer] Write error after flush: " << finalPath << "\n";
        return false;
    }

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
        std::cerr << "[SceneSerializer] cwd = " << fs::current_path().string() << "\n";
        return false;
    }

    // Remove all current objects
    {
        std::vector<std::uint64_t> ids;
        ids.reserve(scene.getObjects().size());
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
        {
            std::cerr << "[SceneSerializer] Skipping bad line: " << line << "\n";
            continue;
        }

        MeshType meshType = MeshType::Cube;
        if (mesh == static_cast<int>(MeshType::Plane))
            meshType = MeshType::Plane;
        else if (mesh == static_cast<int>(MeshType::ImportedModel))
            meshType = MeshType::ImportedModel;

        MaterialType matType = MaterialType::Default;
        if (mat == static_cast<int>(MaterialType::Ground))
            matType = MaterialType::Ground;

        scene.createObject(
            name,
            glm::vec3(px, py, pz),
            glm::vec3(sx, sy, sz),
            glm::vec3(cr, cg, cb),
            0.0f,
            meshType,
            matType
        );
        ++loaded;
    }

    std::cout << "[SceneSerializer] Loaded " << loaded
        << " objects <- " << finalPath << "\n";
    return loaded > 0 || true; // empty scene file is still "success"
}