
#pragma once

#include "Camera.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "ObjLoader.hpp"
#include "Scene.hpp"
#include "Shader.hpp"

#include <memory>

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
        int viewportHeight
    );

private:
    Shader shader;

    Mesh cubeMesh;
    Mesh planeMesh;

    Material defaultMaterial;
    Material groundMaterial;

    std::unique_ptr<Mesh> importedMesh;
};
