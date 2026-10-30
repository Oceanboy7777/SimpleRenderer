#include "SceneSerializer.hpp"
#include "GameObject.hpp"

#include <fstream>
#include <sstream>
#include <iostream>

bool SceneSerializer::save(const Scene& scene, const std::string& path)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open scene for writing: " << path << "\n";
        return false;
    }

    file << "# SimpleRenderer scene v1\n";

    for (const auto& obj : scene.getObjects())
    {
        // Skip nothing — save everything including Ground
        const glm::vec3 p = obj->getPosition();
        const glm::vec3 s = obj->getScale();
        const glm::vec3 c = obj->getColor();

        int mesh = static_cast<int>(obj->getMeshType());
        int mat = static_cast<int>(obj->getMaterialType());

        // name can contain spaces → wrap in quotes
        file << "object \"" << obj->getName() << "\" "
            << p.x << " " << p.y << " " << p.z << " "
            << s.x << " " << s.y << " " << s.z << " "
            << c.r << " " << c.g << " " << c.b << " "
            << mesh << " " << mat << "\n";
    }

    std::cout << "Scene saved: " << path << "\n";
    return true;
}

bool SceneSerializer::load(Scene& scene, const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open scene for reading: " << path << "\n";
        return false;
    }

    // Clear existing objects by removing them one by one
    // (we collect ids first because we can't mutate while iterating easily)
    {
        std::vector<std::uint64_t> ids;
        for (const auto& obj : scene.getObjects())
            ids.push_back(obj->getId());
        for (std::uint64_t id : ids)
            scene.removeObject(id);
    }

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

        // Read quoted name
        std::string name;
        ss >> std::ws;
        if (ss.peek() == '"')
        {
            ss.get(); // skip "
            std::getline(ss, name, '"');
        }
        else
        {
            ss >> name;
        }

        float px, py, pz, sx, sy, sz, cr, cg, cb;
        int mesh, mat;
        if (!(ss >> px >> py >> pz >> sx >> sy >> sz >> cr >> cg >> cb >> mesh >> mat))
            continue;

        MeshType meshType = MeshType::Cube;
        if (mesh == static_cast<int>(MeshType::Plane)) meshType = MeshType::Plane;
        else if (mesh == static_cast<int>(MeshType::ImportedModel)) meshType = MeshType::ImportedModel;

        MaterialType matType = MaterialType::Default;
        if (mat == static_cast<int>(MaterialType::Ground)) matType = MaterialType::Ground;

        scene.createObject(
            name,
            glm::vec3(px, py, pz),
            glm::vec3(sx, sy, sz),
            glm::vec3(cr, cg, cb),
            0.0f,          // no auto-spin on load
            meshType,
            matType
        );
    }

    std::cout << "Scene loaded: " << path << "\n";
    return true;
}