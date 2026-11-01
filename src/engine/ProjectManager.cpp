#include "ProjectManager.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

ProjectInfo ProjectManager::s_current{};
bool ProjectManager::s_hasProject = false;

static fs::path engineRoot()
{
#ifdef SIMPLERENDERER_PROJECT_ROOT
    return fs::path(SIMPLERENDERER_PROJECT_ROOT);
#elif defined(SIMPLERENDERER_ASSET_DIR)
    return fs::path(SIMPLERENDERER_ASSET_DIR).parent_path();
#else
    return fs::current_path();
#endif
}

fs::path ProjectManager::projectsRoot()
{
    return engineRoot() / "Projects";
}

void ProjectManager::ensureProjectsRoot()
{
    std::error_code ec;
    fs::create_directories(projectsRoot(), ec);
}

std::vector<ProjectInfo> ProjectManager::listProjects()
{
    ensureProjectsRoot();
    std::vector<ProjectInfo> out;

    for (auto& e : fs::directory_iterator(projectsRoot()))
    {
        if (!e.is_directory()) continue;
        const std::string folder = e.path().filename().string();
        // Expect Project1, Project2, ...
        if (folder.rfind("Project", 0) != 0) continue;

        ProjectInfo info;
        info.rootPath = e.path();
        try { info.id = std::stoi(folder.substr(7)); }
        catch (...) { info.id = 0; }

        fs::path meta = e.path() / "project.txt";
        if (fs::exists(meta))
        {
            std::ifstream f(meta);
            std::getline(f, info.name);
        }
        if (info.name.empty())
            info.name = folder;

        out.push_back(info);
    }

    std::sort(out.begin(), out.end(),
        [](const ProjectInfo& a, const ProjectInfo& b) { return a.id < b.id; });
    return out;
}

ProjectInfo ProjectManager::createProject(const std::string& name)
{
    ensureProjectsRoot();
    auto existing = listProjects();
    int nextId = 1;
    for (const auto& p : existing)
        nextId = std::max(nextId, p.id + 1);

    ProjectInfo info;
    info.id = nextId;
    info.name = name.empty() ? ("Project" + std::to_string(nextId)) : name;
    info.rootPath = projectsRoot() / ("Project" + std::to_string(nextId));

    fs::create_directories(info.rootPath / "Scenes");
    fs::create_directories(info.rootPath / "Assets" / "Textures");
    fs::create_directories(info.rootPath / "Assets" / "Models");

    {
        std::ofstream f(info.rootPath / "project.txt");
        f << info.name << "\n";
    }

    // Default empty scene
    {
        std::ofstream f(info.rootPath / "Scenes" / "main.scene");
        f << "# SimpleRenderer scene v1\n";
    }

    openProject(info);
    std::cout << "[Project] Created " << info.rootPath << " (" << info.name << ")\n";
    return info;
}

bool ProjectManager::openProject(const ProjectInfo& info)
{
    if (!fs::exists(info.rootPath)) return false;
    s_current = info;
    s_hasProject = true;
    std::cout << "[Project] Opened " << info.rootPath << "\n";
    return true;
}

const ProjectInfo* ProjectManager::current()
{
    return s_hasProject ? &s_current : nullptr;
}

fs::path ProjectManager::scenesDir()
{
    return s_hasProject ? s_current.rootPath / "Scenes" : fs::path{};
}
fs::path ProjectManager::assetsDir()
{
    return s_hasProject ? s_current.rootPath / "Assets" : fs::path{};
}
fs::path ProjectManager::texturesDir()
{
    return assetsDir() / "Textures";
}
fs::path ProjectManager::modelsDir()
{
    return assetsDir() / "Models";
}

std::vector<fs::path> ProjectManager::listSceneFiles()
{
    std::vector<fs::path> out;
    if (!s_hasProject) return out;
    for (auto& e : fs::directory_iterator(scenesDir()))
    {
        if (e.is_regular_file() && e.path().extension() == ".scene")
            out.push_back(e.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::string ProjectManager::nextSceneName(const std::string& base)
{
    auto scenes = listSceneFiles();
    int n = 1;
    while (true)
    {
        std::string candidate = base + std::to_string(n) + ".scene";
        bool exists = false;
        for (const auto& s : scenes)
            if (s.filename().string() == candidate) { exists = true; break; }
        if (!exists) return candidate;
        ++n;
    }
}