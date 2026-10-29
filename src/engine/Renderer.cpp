
#include "Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>

namespace
{
    constexpr const char* vertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 worldPosition;
        out vec3 worldNormal;
        out vec2 textureCoordinates;

        void main()
        {
            vec4 positionInWorld = model * vec4(aPosition, 1.0);

            worldPosition = positionInWorld.xyz;

            // Correctly transform normals when objects are rotated or scaled.
            worldNormal =
                mat3(transpose(inverse(model))) * aNormal;

            textureCoordinates = aTexCoord;

            gl_Position =
                projection * view * positionInWorld;
        }
    )";

    constexpr const char* fragmentShaderSource = R"(
        #version 330 core

        in vec3 worldPosition;
        in vec3 worldNormal;
        in vec2 textureCoordinates;

        uniform vec3 objectColor;
        uniform vec3 materialTint;

        uniform vec3 lightPosition;
        uniform vec3 lightColor;
        uniform vec3 viewPosition;

        uniform sampler2D diffuseTexture;

        out vec4 fragmentColor;

        void main()
        {
            // Surface and lighting directions.
            vec3 normal = normalize(worldNormal);

            vec3 toLight = lightPosition - worldPosition;
            float distanceToLight = length(toLight);
            vec3 lightDirection = normalize(toLight);

            vec3 viewDirection =
                normalize(viewPosition - worldPosition);

            // Ambient lighting: the minimum light reaching the surface.
            float ambientStrength = 0.22;

            vec3 ambient =
                ambientStrength * lightColor;

            // Diffuse lighting: stronger when the surface faces the light.
            float diffuseStrength = max(
                dot(normal, lightDirection),
                0.0
            );

            vec3 diffuse =
                diffuseStrength * lightColor;

            // Specular lighting: creates highlights based on the camera angle.
            vec3 halfwayDirection = normalize(
                lightDirection + viewDirection
            );

            float specularStrength = pow(
                max(dot(normal, halfwayDirection), 0.0),
                32.0
            );

            vec3 specular =
                0.35 * specularStrength * lightColor;

            // The point light becomes weaker as the distance increases.
            float attenuation = 1.0 / (
                1.0
                + 0.045 * distanceToLight
                + 0.0075 * distanceToLight * distanceToLight
            );

            // Sample the existing texture and apply the material colours.
            vec4 sampledTexture = texture(
                diffuseTexture,
                textureCoordinates
            );

            vec3 baseColor =
                sampledTexture.rgb
                * objectColor
                * materialTint;

            // Combine the lighting components.
            vec3 lighting =
                ambient
                + attenuation * (diffuse + specular);

            vec3 finalColor = baseColor * lighting;

            fragmentColor = vec4(
                finalColor,
                sampledTexture.a
            );
        }
    )";

    // Each vertex contains:
    // position XYZ, normal XYZ, texture U and texture V.
    constexpr float cubeVertices[] =
    {
        // Back face: normal (0, 0, -1).
        -1, -1, -1,  0, 0, -1,  0, 0,
         1, -1, -1,  0, 0, -1,  1, 0,
         1,  1, -1,  0, 0, -1,  1, 1,
        -1,  1, -1,  0, 0, -1,  0, 1,

        // Front face: normal (0, 0, 1).
        -1, -1,  1,  0, 0, 1,  0, 0,
         1, -1,  1,  0, 0, 1,  1, 0,
         1,  1,  1,  0, 0, 1,  1, 1,
        -1,  1,  1,  0, 0, 1,  0, 1,

        // Left face: normal (-1, 0, 0).
        -1, -1,  1, -1, 0, 0,  0, 0,
        -1, -1, -1, -1, 0, 0,  1, 0,
        -1,  1, -1, -1, 0, 0,  1, 1,
        -1,  1,  1, -1, 0, 0,  0, 1,

        // Right face: normal (1, 0, 0).
         1, -1, -1,  1, 0, 0,  0, 0,
         1, -1,  1,  1, 0, 0,  1, 0,
         1,  1,  1,  1, 0, 0,  1, 1,
         1,  1, -1,  1, 0, 0,  0, 1,

         // Top face: normal (0, 1, 0).
         -1,  1, -1,  0, 1, 0,  0, 0,
          1,  1, -1,  0, 1, 0,  1, 0,
          1,  1,  1,  0, 1, 0,  1, 1,
         -1,  1,  1,  0, 1, 0,  0, 1,

         // Bottom face: normal (0, -1, 0).
         -1, -1,  1,  0, -1, 0,  0, 0,
          1, -1,  1,  0, -1, 0,  1, 0,
          1, -1, -1,  0, -1, 0,  1, 1,
         -1, -1, -1,  0, -1, 0,  0, 1
    };

    constexpr unsigned int cubeIndices[] =
    {
         0,  1,  2,   2,  3,  0,
         4,  5,  6,   6,  7,  4,
         8,  9, 10,  10, 11,  8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20
    };

    // A horizontal plane with an upward-facing normal.
    constexpr float planeVertices[] =
    {
        -0.5f, 0.0f, -0.5f,  0, 1, 0,  0, 0,
         0.5f, 0.0f, -0.5f,  0, 1, 0,  1, 0,
         0.5f, 0.0f,  0.5f,  0, 1, 0,  1, 1,
        -0.5f, 0.0f,  0.5f,  0, 1, 0,  0, 1
    };

    constexpr unsigned int planeIndices[] =
    {
        0, 1, 2,
        2, 3, 0
    };
}

Renderer::Renderer()
    : shader(vertexShaderSource, fragmentShaderSource),
    cubeMesh(
        cubeVertices,
        sizeof(cubeVertices) / sizeof(float),
        cubeIndices,
        sizeof(cubeIndices) / sizeof(unsigned int)
    ),
    planeMesh(
        planeVertices,
        sizeof(planeVertices) / sizeof(float),
        planeIndices,
        sizeof(planeIndices) / sizeof(unsigned int)
    ),
    defaultMaterial(
        "Default",
        "assets/textures/cube.png",
        glm::vec3(1.0f)
    ),
    groundMaterial(
        "Ground",
        "assets/textures/ground.png",
        glm::vec3(0.65f, 0.8f, 0.65f)
    ),
    importedMesh(nullptr)
{
    ObjMeshData modelData;
    std::string errorMessage;

    if (ObjLoader::load("assets/models/sample_pyramid.obj", modelData, errorMessage))
    {
        importedMesh = std::make_unique<Mesh>(
            modelData.vertices.data(),
            modelData.vertices.size(),
            modelData.indices.data(),
            modelData.indices.size()
        );
    }
    else
    {
        std::cerr << "Model import failed: " << errorMessage << '\n';
    }
}

Renderer::~Renderer() = default;


bool Renderer::isValid() const
{
    return shader.isValid()
        && cubeMesh.isValid()
        && planeMesh.isValid()
        && defaultMaterial.isValid()
        && groundMaterial.isValid()
        && importedMesh != nullptr
        && importedMesh->isValid();
}

void Renderer::render(
    const Scene& scene,
    const Camera& camera,
    int viewportWidth,
    int viewportHeight
)
{
    if (viewportWidth <= 0 || viewportHeight <= 0)
    {
        return;
    }

    glViewport(0, 0, viewportWidth, viewportHeight);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspectRatio =
        static_cast<float>(viewportWidth) /
        static_cast<float>(viewportHeight);

    const glm::mat4 view = camera.getViewMatrix();

    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.getFieldOfView()),
        aspectRatio,
        0.1f,
        100.0f
    );

    shader.use();

    shader.setMatrix4(
        "view",
        glm::value_ptr(view)
    );

    shader.setMatrix4(
        "projection",
        glm::value_ptr(projection)
    );

    // Point light position in world space.
    shader.setVector3(
        "lightPosition",
        -3.0f,
        6.0f,
        3.0f
    );

    // Slightly warm-white light.
    shader.setVector3(
        "lightColor",
        1.0f,
        0.96f,
        0.90f
    );

    // The specular calculation needs the current camera position.
    const glm::vec3 cameraPosition = camera.getPosition();

    shader.setVector3(
        "viewPosition",
        cameraPosition.x,
        cameraPosition.y,
        cameraPosition.z
    );

    shader.setInteger("diffuseTexture", 0);

    for (const auto& object : scene.getObjects())
    {
        const glm::mat4 model =
            scene.getWorldMatrix(object->getId());

        shader.setMatrix4(
            "model",
            glm::value_ptr(model)
        );

        const glm::vec3 color = object->getColor();

        shader.setVector3(
            "objectColor",
            color.r,
            color.g,
            color.b
        );

        const Material* material = &defaultMaterial;

        if (object->getMaterialType() == MaterialType::Ground)
        {
            material = &groundMaterial;
        }

        material->bind();

        const glm::vec3 tint = material->getTint();

        shader.setVector3(
            "materialTint",
            tint.r,
            tint.g,
            tint.b
        );

        if (object->getMeshType() == MeshType::Plane)
        {
            planeMesh.draw();
        }
        else if (object->getMeshType() == MeshType::ImportedModel)
        {
            if (importedMesh != nullptr)
            {
                importedMesh->draw();
            }
        }
        else
        {
            cubeMesh.draw();
        }
    }
}
