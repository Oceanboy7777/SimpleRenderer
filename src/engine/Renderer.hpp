
#pragma once

#include "Camera.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "ObjLoader.hpp"
#include "Scene.hpp"
#include "Shader.hpp"

#include <glad/gl.h>

#include <memory>
#include <cstdint>

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
        std::uint64_t selectedId = 0
    );

private:
    bool createShadowResources();
    void renderShadowPass(const Scene& scene);
    void drawObject(
        const Scene& scene,
        const GameObject& object,
        const Shader& targetShader
    ) const;

    Shader shader;
    Shader shadowShader;

    Mesh cubeMesh;
    Mesh planeMesh;

    Material defaultMaterial;
    Material groundMaterial;

    std::unique_ptr<Mesh> importedMesh;

    GLuint shadowFramebuffer;
    GLuint shadowDepthTexture;

    int shadowMapSize;
};
