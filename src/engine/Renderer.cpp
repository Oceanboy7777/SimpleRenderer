
#include "Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <iostream>
#include <memory>

namespace
{
    constexpr int shadowMapResolution = 2048;

    constexpr const char* vertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 lightSpaceMatrix;

        out vec3 worldPosition;
        out vec3 worldNormal;
        out vec2 textureCoordinates;
        out vec4 fragmentPositionInLightSpace;

        void main()
        {
            vec4 positionInWorld = model * vec4(aPosition, 1.0);

            worldPosition = positionInWorld.xyz;

            worldNormal =
                mat3(transpose(inverse(model))) * aNormal;

            textureCoordinates = aTexCoord;

            fragmentPositionInLightSpace =
                lightSpaceMatrix * positionInWorld;

            gl_Position =
                projection * view * positionInWorld;
        }
    )";

    constexpr const char* fragmentShaderSource = R"(
        #version 330 core

        in vec3 worldPosition;
        in vec3 worldNormal;
        in vec2 textureCoordinates;
        in vec4 fragmentPositionInLightSpace;

        uniform vec3 objectColor;
        uniform vec3 materialTint;

        // Direction from a surface toward the light.
        uniform vec3 lightDirection;
        uniform vec3 lightColor;
        uniform vec3 viewPosition;

        uniform sampler2D diffuseTexture;
        uniform sampler2D shadowMap;

        out vec4 fragmentColor;

        float calculateShadow(vec3 normal, vec3 directionToLight)
        {
            vec3 projectedPosition =
                fragmentPositionInLightSpace.xyz /
                fragmentPositionInLightSpace.w;

            // Convert light-space coordinates from [-1, 1] to [0, 1].
            projectedPosition =
                projectedPosition * 0.5 + 0.5;

            // Anything outside the shadow map is treated as lit.
            if (projectedPosition.z > 1.0 ||
                projectedPosition.x < 0.0 ||
                projectedPosition.x > 1.0 ||
                projectedPosition.y < 0.0 ||
                projectedPosition.y > 1.0)
            {
                return 0.0;
            }

            float closestDepth = texture(
                shadowMap,
                projectedPosition.xy
            ).r;

            float currentDepth = projectedPosition.z;

            // Slope-aware bias reduces surface shadow artefacts.
            float bias = max(
                0.0025 * (1.0 - dot(normal, directionToLight)),
                0.0005
            );

            return currentDepth - bias > closestDepth
                ? 1.0
                : 0.0;
        }

        void main()
        {
            vec3 normal = normalize(worldNormal);

            vec3 directionToLight = normalize(lightDirection);

            vec3 viewDirection =
                normalize(viewPosition - worldPosition);

            float ambientStrength = 0.22;

            vec3 ambient =
                ambientStrength * lightColor;

            float diffuseStrength = max(
                dot(normal, directionToLight),
                0.0
            );

            vec3 diffuse =
                diffuseStrength * lightColor;

            vec3 halfwayDirection = normalize(
                directionToLight + viewDirection
            );

            float specularStrength = pow(
                max(dot(normal, halfwayDirection), 0.0),
                32.0
            );

            vec3 specular =
                0.35 * specularStrength * lightColor;

            float shadow = calculateShadow(
                normal,
                directionToLight
            );

            // Ambient light remains visible in shadow.
            // Direct diffuse and specular light are shadowed.
            vec3 lighting =
                ambient
                + (1.0 - shadow) * (diffuse + specular);

            vec4 sampledTexture = texture(
                diffuseTexture,
                textureCoordinates
            );

            vec3 baseColor =
                sampledTexture.rgb
                * objectColor
                * materialTint;

            fragmentColor = vec4(
                baseColor * lighting,
                sampledTexture.a
            );
        }
    )";

    constexpr const char* shadowVertexShaderSource = R"(
        #version 330 core

        layout (location = 0) in vec3 aPosition;

        uniform mat4 model;
        uniform mat4 lightSpaceMatrix;

        void main()
        {
            gl_Position =
                lightSpaceMatrix * model * vec4(aPosition, 1.0);
        }
    )";

    constexpr const char* shadowFragmentShaderSource = R"(
        #version 330 core

        void main()
        {
            // Depth is written automatically by OpenGL.
        }
    )";

    // Each vertex contains position XYZ, normal XYZ and UV.
    constexpr float cubeVertices[] =
    {
        // Back face.
        -1, -1, -1,  0, 0, -1,  0, 0,
         1, -1, -1,  0, 0, -1,  1, 0,
         1,  1, -1,  0, 0, -1,  1, 1,
        -1,  1, -1,  0, 0, -1,  0, 1,

        // Front face.
        -1, -1,  1,  0, 0, 1,  0, 0,
         1, -1,  1,  0, 0, 1,  1, 0,
         1,  1,  1,  0, 0, 1,  1, 1,
        -1,  1,  1,  0, 0, 1,  0, 1,

        // Left face.
        -1, -1,  1, -1, 0, 0,  0, 0,
        -1, -1, -1, -1, 0, 0,  1, 0,
        -1,  1, -1, -1, 0, 0,  1, 1,
        -1,  1,  1, -1, 0, 0,  0, 1,

        // Right face.
         1, -1, -1,  1, 0, 0,  0, 0,
         1, -1,  1,  1, 0, 0,  1, 0,
         1,  1,  1,  1, 0, 0,  1, 1,
         1,  1, -1,  1, 0, 0,  0, 1,

         // Top face.
         -1,  1, -1,  0, 1, 0,  0, 0,
          1,  1, -1,  0, 1, 0,  1, 0,
          1,  1,  1,  0, 1, 0,  1, 1,
         -1,  1,  1,  0, 1, 0,  0, 1,

         // Bottom face.
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

    // Direction from the scene toward the light.
    const glm::vec3 sceneLightDirection =
        glm::normalize(glm::vec3(-0.45f, 0.80f, 0.45f));

    glm::mat4 makeLightSpaceMatrix()
    {
        const glm::vec3 lightPosition =
            sceneLightDirection * 25.0f;

        const glm::mat4 lightProjection = glm::ortho(
            -15.0f, 15.0f,
            -15.0f, 15.0f,
            1.0f, 50.0f
        );

        const glm::mat4 lightView = glm::lookAt(
            lightPosition,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        return lightProjection * lightView;
    }
}

Renderer::Renderer()
    : shader(vertexShaderSource, fragmentShaderSource),
    shadowShader(
        shadowVertexShaderSource,
        shadowFragmentShaderSource
    ),
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
    importedMesh(nullptr),
    shadowFramebuffer(0),
    shadowDepthTexture(0),
    shadowMapSize(shadowMapResolution)
{
    ObjMeshData modelData;
    std::string errorMessage;

    if (ObjLoader::load(
        "assets/models/sample_pyramid.obj",
        modelData,
        errorMessage
    ))
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
        std::cerr << "Model import failed: "
            << errorMessage << '\n';
    }

    createShadowResources();
}

Renderer::~Renderer()
{
    if (shadowDepthTexture != 0)
    {
        glDeleteTextures(1, &shadowDepthTexture);
    }

    if (shadowFramebuffer != 0)
    {
        glDeleteFramebuffers(1, &shadowFramebuffer);
    }
}

bool Renderer::createShadowResources()
{
    glGenFramebuffers(1, &shadowFramebuffer);
    glGenTextures(1, &shadowDepthTexture);

    if (shadowFramebuffer == 0 || shadowDepthTexture == 0)
    {
        std::cerr << "Failed to create shadow-map OpenGL resources.\n";
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        shadowMapSize,
        shadowMapSize,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_BORDER
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_BORDER
    );

    constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glTexParameterfv(
        GL_TEXTURE_2D,
        GL_TEXTURE_BORDER_COLOR,
        borderColor
    );

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        shadowDepthTexture,
        0
    );

    // This framebuffer only stores depth, not colour.
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum framebufferStatus =
        glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Shadow framebuffer is incomplete. Status: "
            << framebufferStatus << '\n';
        return false;
    }

    return true;
}

bool Renderer::isValid() const
{
    return shader.isValid()
        && shadowShader.isValid()
        && cubeMesh.isValid()
        && planeMesh.isValid()
        && defaultMaterial.isValid()
        && groundMaterial.isValid()
        && importedMesh != nullptr
        && importedMesh->isValid()
        && shadowFramebuffer != 0
        && shadowDepthTexture != 0;
}

void Renderer::renderShadowPass(const Scene& scene)
{
    const glm::mat4 lightSpaceMatrix = makeLightSpaceMatrix();

    glViewport(
        0,
        0,
        shadowMapSize,
        shadowMapSize
    );

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.use();

    shadowShader.setMatrix4(
        "lightSpaceMatrix",
        glm::value_ptr(lightSpaceMatrix)
    );

    for (const auto& object : scene.getObjects())
    {
        const glm::mat4 model =
            scene.getWorldMatrix(object->getId());

        shadowShader.setMatrix4(
            "model",
            glm::value_ptr(model)
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

    // Restore the normal framebuffer before the main render pass.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::drawObject(
    const Scene& scene,
    const GameObject& object,
    const Shader& targetShader
) const
{
    const glm::mat4 model =
        scene.getWorldMatrix(object.getId());

    targetShader.setMatrix4(
        "model",
        glm::value_ptr(model)
    );

    const glm::vec3 color = object.getColor();

    targetShader.setVector3(
        "objectColor",
        color.r,
        color.g,
        color.b
    );

    const Material* material = &defaultMaterial;

    if (object.getMaterialType() == MaterialType::Ground)
    {
        material = &groundMaterial;
    }

    material->bind();

    const glm::vec3 tint = material->getTint();

    targetShader.setVector3(
        "materialTint",
        tint.r,
        tint.g,
        tint.b
    );

    if (object.getMeshType() == MeshType::Plane)
    {
        planeMesh.draw();
    }
    else if (object.getMeshType() == MeshType::ImportedModel)
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

void Renderer::render(
    const Scene& scene,
    const Camera& camera,
    int viewportWidth,
    int viewportHeight,
    std::uint64_t selectedId   // ← must be here
)
{
    if (viewportWidth <= 0 || viewportHeight <= 0)
    {
        return;
    }

    if (shadowFramebuffer == 0 || shadowDepthTexture == 0)
    {
        return;
    }

    const glm::mat4 lightSpaceMatrix = makeLightSpaceMatrix();

    // Pass 1: render scene depth from the light's perspective.
    renderShadowPass(scene);

    // Pass 2: render the scene from the camera's perspective.
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

    shader.setMatrix4(
        "lightSpaceMatrix",
        glm::value_ptr(lightSpaceMatrix)
    );

    shader.setVector3(
        "lightDirection",
        sceneLightDirection.x,
        sceneLightDirection.y,
        sceneLightDirection.z
    );

    shader.setVector3(
        "lightColor",
        1.0f,
        0.96f,
        0.90f
    );

    const glm::vec3 cameraPosition = camera.getPosition();

    shader.setVector3(
        "viewPosition",
        cameraPosition.x,
        cameraPosition.y,
        cameraPosition.z
    );

    // The object's diffuse texture uses texture unit zero.
    shader.setInteger("diffuseTexture", 0);

    // The shadow depth map uses texture unit one.
    shader.setInteger("shadowMap", 1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);

    glActiveTexture(GL_TEXTURE0);

    for (const auto& object : scene.getObjects())
    {
        drawObject(scene, *object, shader);
    }

    // Leave OpenGL in a predictable texture state.
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    // ===== Selection outline (Edit mode) =====
    if (selectedId != 0)
    {
        const GameObject* selected = scene.findObjectById(selectedId);
        if (selected != nullptr)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.5f);
            glDisable(GL_DEPTH_TEST);

            const glm::mat4 model = scene.getWorldMatrix(selected->getId());
            shader.setMatrix4("model", glm::value_ptr(model));
            shader.setVector3("objectColor", 1.0f, 0.85f, 0.2f);
            shader.setVector3("materialTint", 1.0f, 1.0f, 1.0f);
            defaultMaterial.bind();

            if (selected->getMeshType() == MeshType::Plane)
                planeMesh.draw();
            else if (selected->getMeshType() == MeshType::ImportedModel && importedMesh)
                importedMesh->draw();
            else
                cubeMesh.draw();

            glEnable(GL_DEPTH_TEST);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glLineWidth(1.0f);
        }
    }
} // ← this closes Renderer::render
