#pragma once

#include "Camera.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "ObjLoader.hpp"
#include "Scene.hpp"
#include "Shader.hpp"

#include "editor/TransformToolbar.hpp"
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
        GizmoMode gizmoMode = GizmoMode::Translate
    );

private:
    bool createShadowResources();
    void renderShadowPass(const Scene& scene);
    void drawObject(
        const Scene& scene,
        const GameObject& object,
        const Shader& targetShader
    );

    void drawObjectMesh(const GameObject& object);
    void drawGrid(const Camera& camera, int viewportWidth, int viewportHeight);
    Mesh* getOrLoadMesh(const std::string& path);

    void drawGrid(const Camera& camera, int viewportWidth, int viewportHeight);
    void drawGizmo(const Camera& camera, const glm::vec3& position, GizmoMode mode, int viewportWidth, int viewportHeight);

    Shader shader;
    Shader shadowShader;

    Mesh cubeMesh;
    Mesh planeMesh;

    Material defaultMaterial;
    Material groundMaterial;

    std::unique_ptr<Mesh> importedMesh;

    // Cache of custom .obj meshes by absolute/relative path
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshCache;

    GLuint shadowFramebuffer;
    GLuint shadowDepthTexture;

    int shadowMapSize;
};