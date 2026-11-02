#pragma once

#include "Camera.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "ObjLoader.hpp"
#include "Scene.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "editor/GizmoMode.hpp"

#include <glad/gl.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class Renderer
{
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool isValid() const;

    void render(
        const Scene& scene,
        const Camera& camera,
        int viewportWidth,
        int viewportHeight,
        std::uint64_t selectedId = 0,
        GizmoMode gizmoMode = GizmoMode::Translate,
        bool showGrid = true
    );

private:
    bool createShadowResources();
    void renderShadowPass(const Scene& scene);
    void drawObject(
        const Scene& scene,
        const GameObject& object,
        const Shader& targetShader
    );

    Mesh* getOrLoadMesh(const std::string& path);
    Texture* getOrLoadTexture(const std::string& path);
    const Mesh& meshForObject(const GameObject& object);

    struct MeshBounds { glm::vec3 minB{ -1 }; glm::vec3 maxB{ 1 }; };
    std::unordered_map<std::string, MeshBounds> meshBounds;
    bool tryGetMeshBounds(const std::string& path, glm::vec3& outMin, glm::vec3& outMax) const;

    void drawGrid(const Camera& camera, int viewportWidth, int viewportHeight);
    void drawGizmo(
        const Camera& camera,
        const glm::vec3& position,
        GizmoMode mode,
        int viewportWidth,
        int viewportHeight
    );
    void drawSelectionOutline(
        const Scene& scene,
        const GameObject& object,
        const Camera& camera,
        int viewportWidth,
        int viewportHeight
    );

    Shader shader;
    Shader shadowShader;
    Shader unlitShader;
    Shader gridShader;

    Mesh cubeMesh;
    Mesh planeMesh;

    Material defaultMaterial;
    Material groundMaterial;

    std::unique_ptr<Mesh> importedMesh;
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshCache;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textureCache;

    GLuint shadowFramebuffer;
    GLuint shadowDepthTexture;
    int shadowMapSize;
};