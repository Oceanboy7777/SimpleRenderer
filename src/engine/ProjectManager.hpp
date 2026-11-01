#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

struct ProjectInfo
{
    int id = 0;
    std::string name;
    fs::path rootPath;   // .../Projects/Project1
};

class ProjectManager
{
public:
    static fs::path projectsRoot();          // <repo>/Projects
    static void ensureProjectsRoot();

    static std::vector<ProjectInfo> listProjects();
    static ProjectInfo createProject(const std::string& name); // empty name → ProjectN
    static bool openProject(const ProjectInfo& info);

    static const ProjectInfo* current();
    static fs::path scenesDir();
    static fs::path assetsDir();
    static fs::path texturesDir();
    static fs::path modelsDir();

    static std::vector<fs::path> listSceneFiles();
    static std::string nextSceneName(const std::string& base = "Scene");

private:
    static ProjectInfo s_current;
    static bool s_hasProject;
};