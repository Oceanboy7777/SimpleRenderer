#include "Renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <limits>
#include <unordered_map>
#include <string>
#include <vector>

namespace
{
    constexpr int shadowMapResolution = 2048;

    // Axis colors: X #B700FF  Y #00FFB7  Z #FFB700
    const glm::vec3 kAxisX(0xB7 / 255.0f, 0x00 / 255.0f, 0xFF / 255.0f);
    const glm::vec3 kAxisY(0x00 / 255.0f, 0xFF / 255.0f, 0xB7 / 255.0f);
    const glm::vec3 kAxisZ(0xFF / 255.0f, 0xB7 / 255.0f, 0x00 / 255.0f);

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
            worldNormal = mat3(transpose(inverse(model))) * aNormal;
            textureCoordinates = aTexCoord;
            fragmentPositionInLightSpace = lightSpaceMatrix * positionInWorld;
            gl_Position = projection * view * positionInWorld;
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
        uniform vec3 lightDirection;
        uniform vec3 lightColor;
        uniform vec3 viewPosition;
        uniform sampler2D diffuseTexture;
        uniform sampler2D shadowMap;
        out vec4 fragmentColor;
        float calculateShadow(vec3 normal, vec3 directionToLight)
        {
            vec3 projectedPosition = fragmentPositionInLightSpace.xyz / fragmentPositionInLightSpace.w;
            projectedPosition = projectedPosition * 0.5 + 0.5;
            if (projectedPosition.z > 1.0 || projectedPosition.x < 0.0 || projectedPosition.x > 1.0 ||
                projectedPosition.y < 0.0 || projectedPosition.y > 1.0)
                return 0.0;
            float closestDepth = texture(shadowMap, projectedPosition.xy).r;
            float currentDepth = projectedPosition.z;
            float bias = max(0.0025 * (1.0 - dot(normal, directionToLight)), 0.0005);
            return currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
        void main()
        {
            vec3 normal = normalize(worldNormal);
            vec3 directionToLight = normalize(lightDirection);
            vec3 viewDirection = normalize(viewPosition - worldPosition);
            vec3 ambient = 0.22 * lightColor;
            vec3 diffuse = max(dot(normal, directionToLight), 0.0) * lightColor;
            vec3 halfwayDirection = normalize(directionToLight + viewDirection);
            vec3 specular = 0.35 * pow(max(dot(normal, halfwayDirection), 0.0), 32.0) * lightColor;
            float shadow = calculateShadow(normal, directionToLight);
            vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);
            vec4 sampledTexture = texture(diffuseTexture, textureCoordinates);
            vec3 baseColor = sampledTexture.rgb * objectColor * materialTint;
            fragmentColor = vec4(baseColor * lighting, sampledTexture.a);
        }
    )";

    constexpr const char* shadowVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPosition;
        uniform mat4 model;
        uniform mat4 lightSpaceMatrix;
        void main()
        {
            gl_Position = lightSpaceMatrix * model * vec4(aPosition, 1.0);
        }
    )";

    constexpr const char* shadowFragmentShaderSource = R"(
        #version 330 core
        void main() {}
    )";

    constexpr const char* unlitVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
        }
    )";

    constexpr const char* unlitFragmentShaderSource = R"(
        #version 330 core
        uniform vec3 objectColor;
        out vec4 fragmentColor;
        void main()
        {
            fragmentColor = vec4(objectColor, 1.0);
        }
    )";

    // Procedural infinite-ish grid on a large plane, fades with distance
    constexpr const char* gridVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 worldPosition;
        void main()
        {
            vec4 wp = model * vec4(aPosition, 1.0);
            worldPosition = wp.xyz;
            gl_Position = projection * view * wp;
        }
    )";

    constexpr const char* gridFragmentShaderSource = R"(
        #version 330 core
        in vec3 worldPosition;
        uniform vec3 cameraPosition;
        out vec4 fragmentColor;

        float gridLine(vec2 pos, float scale, float thickness)
        {
            vec2 c = abs(fract(pos / scale - 0.5) - 0.5);
            vec2 d = fwidth(pos / scale);
            vec2 g = smoothstep(thickness * d, 2.0 * thickness * d, c);
            return 1.0 - min(g.x, g.y);
        }

        void main()
        {
            float gridScale = 1.0;
            vec2 pos = worldPosition.xz;

            float minor = gridLine(pos, gridScale, 0.8);
            float major = gridLine(pos, gridScale * 10.0, 1.0);

            // Barely visible
            float line = minor * 0.12 + major * 0.22;

            // Axis lines slightly stronger
            float axisX = (abs(pos.y) < 0.02 * gridScale) ? 0.35 : 0.0; // Z=0 -> X axis in xz (pos.y is z)
            float axisZ = (abs(pos.x) < 0.02 * gridScale) ? 0.35 : 0.0;

            vec3 color = vec3(0.55, 0.58, 0.62);
            if (axisZ > 0.0) color = vec3(0.72, 0.0, 1.0);   // X #B700FF-ish
            if (axisX > 0.0) color = vec3(1.0, 0.72, 0.0);    // Z #FFB700-ish

            float alpha = max(line, max(axisX, axisZ));

            float dist = length(cameraPosition.xz - pos);
            float fade = 1.0 - smoothstep(40.0, 120.0, dist);
            alpha *= fade * 0.85;

            if (alpha < 0.01)
                discard;

            fragmentColor = vec4(color, alpha);
        }
    )";

    constexpr float cubeVertices[] =
    {
        -1, -1, -1,  0, 0, -1,  0, 0,   1, -1, -1,  0, 0, -1,  1, 0,
         1,  1, -1,  0, 0, -1,  1, 1,  -1,  1, -1,  0, 0, -1,  0, 1,
        -1, -1,  1,  0, 0,  1,  0, 0,   1, -1,  1,  0, 0,  1,  1, 0,
         1,  1,  1,  0, 0,  1,  1, 1,  -1,  1,  1,  0, 0,  1,  0, 1,
        -1, -1,  1, -1, 0,  0,  0, 0,  -1, -1, -1, -1, 0,  0,  1, 0,
        -1,  1, -1, -1, 0,  0,  1, 1,  -1,  1,  1, -1, 0,  0,  0, 1,
         1, -1, -1,  1, 0,  0,  0, 0,   1, -1,  1,  1, 0,  0,  1, 0,
         1,  1,  1,  1, 0,  0,  1, 1,   1,  1, -1,  1, 0,  0,  0, 1,
        -1,  1, -1,  0, 1,  0,  0, 0,   1,  1, -1,  0, 1,  0,  1, 0,
         1,  1,  1,  0, 1,  0,  1, 1,  -1,  1,  1,  0, 1,  0,  0, 1,
        -1, -1,  1,  0,-1,  0,  0, 0,   1, -1,  1,  0,-1,  0,  1, 0,
         1, -1, -1,  0,-1,  0,  1, 1,  -1, -1, -1,  0,-1,  0,  0, 1
    };

    constexpr unsigned int cubeIndices[] =
    {
         0,1,2, 2,3,0,  4,5,6, 6,7,4,
         8,9,10,10,11,8, 12,13,14,14,15,12,
        16,17,18,18,19,16, 20,21,22,22,23,20
    };

    constexpr float planeVertices[] =
    {
        -0.5f, 0.0f, -0.5f,  0,1,0,  0,0,
         0.5f, 0.0f, -0.5f,  0,1,0,  1,0,
         0.5f, 0.0f,  0.5f,  0,1,0,  1,1,
        -0.5f, 0.0f,  0.5f,  0,1,0,  0,1
    };

    constexpr unsigned int planeIndices[] = { 0,1,2, 2,3,0 };

    const glm::vec3 sceneLightDirection =
        glm::normalize(glm::vec3(-0.45f, 0.80f, 0.45f));

    glm::mat4 makeLightSpaceMatrix()
    {
        const glm::vec3 lightPosition = sceneLightDirection * 25.0f;
        const glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 50.0f);
        const glm::mat4 lightView = glm::lookAt(lightPosition, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        return lightProjection * lightView;
    }
}

Renderer::Renderer()
    : shader(vertexShaderSource, fragmentShaderSource),
    shadowShader(shadowVertexShaderSource, shadowFragmentShaderSource),
    unlitShader(unlitVertexShaderSource, unlitFragmentShaderSource),
    gridShader(gridVertexShaderSource, gridFragmentShaderSource),
    cubeMesh(cubeVertices, sizeof(cubeVertices) / sizeof(float),
        cubeIndices, sizeof(cubeIndices) / sizeof(unsigned int)),
    planeMesh(planeVertices, sizeof(planeVertices) / sizeof(float),
        planeIndices, sizeof(planeIndices) / sizeof(unsigned int)),
    defaultMaterial("Default", "assets/textures/cube.png", glm::vec3(1.0f)),
    groundMaterial("Ground", "assets/textures/ground.png", glm::vec3(0.65f, 0.8f, 0.65f)),
    importedMesh(nullptr),
    shadowFramebuffer(0),
    shadowDepthTexture(0),
    shadowMapSize(shadowMapResolution)
{
    ObjMeshData modelData;
    std::string errorMessage;
    if (ObjLoader::load("assets/models/sample_pyramid.obj", modelData, errorMessage))
    {
        importedMesh = std::make_unique<Mesh>(
            modelData.vertices.data(), modelData.vertices.size(),
            modelData.indices.data(), modelData.indices.size());
    }
    else
    {
        std::cerr << "Model import failed: " << errorMessage << '\n';
    }
    createShadowResources();
}

Renderer::~Renderer()
{
    if (shadowDepthTexture != 0)
        glDeleteTextures(1, &shadowDepthTexture);
    if (shadowFramebuffer != 0)
        glDeleteFramebuffers(1, &shadowFramebuffer);
}

bool Renderer::createShadowResources()
{
    glGenFramebuffers(1, &shadowFramebuffer);
    glGenTextures(1, &shadowDepthTexture);
    if (shadowFramebuffer == 0 || shadowDepthTexture == 0)
        return false;

    glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowMapSize, shadowMapSize,
        0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

bool Renderer::isValid() const
{
    return shader.isValid() && shadowShader.isValid()
        && unlitShader.isValid() && gridShader.isValid()
        && cubeMesh.isValid() && planeMesh.isValid()
        && defaultMaterial.isValid() && groundMaterial.isValid()
        && importedMesh != nullptr && importedMesh->isValid()
        && shadowFramebuffer != 0 && shadowDepthTexture != 0;
}

void Renderer::renderShadowPass(const Scene& scene)
{
    const glm::mat4 lightSpaceMatrix = makeLightSpaceMatrix();
    glViewport(0, 0, shadowMapSize, shadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    shadowShader.use();
    shadowShader.setMatrix4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));

    for (const auto& object : scene.getObjects())
    {
        const glm::mat4 model = scene.getWorldMatrix(object->getId());
        shadowShader.setMatrix4("model", glm::value_ptr(model));
        meshForObject(*object).draw();   // ONLY this
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Mesh* Renderer::getOrLoadMesh(const std::string& path)
{
    if (path.empty())
        return nullptr;

    auto it = meshCache.find(path);
    if (it != meshCache.end())
        return it->second.get();

    ObjMeshData data;
    std::string error;
    if (!ObjLoader::load(path, data, error))
    {
        std::cerr << "[Renderer] Mesh load failed: " << path << " :: " << error << "\n";
        return nullptr;
    }

    MeshBounds b;
    b.minB = glm::vec3(std::numeric_limits<float>::max());
    b.maxB = glm::vec3(std::numeric_limits<float>::lowest());
    // vertices: pos.xyz, normal, uv → stride 8 floats
    for (size_t i = 0; i + 2 < data.vertices.size(); i += 8)
    {
        glm::vec3 p(data.vertices[i], data.vertices[i + 1], data.vertices[i + 2]);
        b.minB = glm::min(b.minB, p);
        b.maxB = glm::max(b.maxB, p);
    }
    meshBounds[path] = b;

    auto mesh = std::make_unique<Mesh>(
        data.vertices.data(),
        data.vertices.size(),
        data.indices.data(),
        data.indices.size()
    );

    if (!mesh->isValid())
        return nullptr;

    Mesh* raw = mesh.get();
    meshCache.emplace(path, std::move(mesh));
    std::cout << "[Renderer] Cached mesh: " << path << "\n";
    return raw;
}

bool Renderer::tryGetMeshBounds(const std::string& path, glm::vec3& outMin, glm::vec3& outMax) const
{
    auto it = meshBounds.find(path);
    if (it == meshBounds.end())
        return false;
    outMin = it->second.minB;
    outMax = it->second.maxB;
    return true;
}

Texture* Renderer::getOrLoadTexture(const std::string& path)
{
    if (path.empty())
        return nullptr;

    auto it = textureCache.find(path);
    if (it != textureCache.end())
        return it->second.get();

    auto tex = std::make_unique<Texture>(path, true);
    if (!tex->isValid())
    {
        std::cerr << "[Renderer] Texture load failed: " << path << "\n";
        return nullptr;
    }

    Texture* raw = tex.get();
    textureCache.emplace(path, std::move(tex));
    std::cout << "[Renderer] Cached texture: " << path << "\n";
    return raw;
}

const Mesh& Renderer::meshForObject(const GameObject& object)
{
    if (!object.getMeshPath().empty())
    {
        if (Mesh* custom = getOrLoadMesh(object.getMeshPath()))
            return *custom;
    }

    if (object.getMeshType() == MeshType::Plane)
        return planeMesh;

    if (object.getMeshType() == MeshType::ImportedModel && importedMesh)
        return *importedMesh;

    return cubeMesh;
}

void Renderer::drawObject(
    const Scene& scene,
    const GameObject& object,
    const Shader& targetShader)
{
    // UI objects are not 3D meshes — never draw them as cubes
    if (object.isUI())
        return;

    const glm::mat4 model = scene.getWorldMatrix(object.getId());
    targetShader.setMatrix4("model", glm::value_ptr(model));

    const glm::vec3 color = object.getColor();
    targetShader.setVector3("objectColor", color.r, color.g, color.b);

    // Custom PNG material
    if (!object.getTexturePath().empty())
    {
        if (Texture* customTex = getOrLoadTexture(object.getTexturePath()))
        {
            customTex->bind(0);
            targetShader.setVector3("materialTint", 1.0f, 1.0f, 1.0f);
            meshForObject(object).draw();
            return;
        }
    }

    const Material* material = &defaultMaterial;
    if (object.getMaterialType() == MaterialType::Ground)
        material = &groundMaterial;

    material->bind();
    const glm::vec3 tint = material->getTint();
    targetShader.setVector3("materialTint", tint.r, tint.g, tint.b);

    meshForObject(object).draw();
}

void Renderer::drawGrid(
    const Camera& camera,
    int viewportWidth,
    int viewportHeight)
{
    if (!gridShader.isValid() || !planeMesh.isValid())
        return;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return;

    const float aspect =
        static_cast<float>(viewportWidth) /
        static_cast<float>(std::max(viewportHeight, 1));
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.getFieldOfView()), aspect, 0.1f, 500.0f);
    const glm::mat4 view = camera.getViewMatrix();

    // Huge plane at y=0; lines are procedural in the shader
    glm::mat4 model(1.0f);
    model = glm::scale(model, glm::vec3(400.0f, 1.0f, 400.0f));

    gridShader.use();
    gridShader.setMatrix4("projection", glm::value_ptr(projection));
    gridShader.setMatrix4("view", glm::value_ptr(view));
    gridShader.setMatrix4("model", glm::value_ptr(model));

    const glm::vec3 cam = camera.getPosition();
    gridShader.setVector3("cameraPosition", cam.x, cam.y, cam.z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    planeMesh.draw();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::drawGizmo(
    const Camera& camera,
    const glm::vec3& position,
    GizmoMode mode,
    int viewportWidth,
    int viewportHeight)
{
    if (!unlitShader.isValid() || !cubeMesh.isValid())
        return;
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return;

    const float aspect =
        static_cast<float>(viewportWidth) /
        static_cast<float>(std::max(viewportHeight, 1));
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.getFieldOfView()), aspect, 0.1f, 500.0f);
    const glm::mat4 view = camera.getViewMatrix();

    unlitShader.use();
    unlitShader.setMatrix4("projection", glm::value_ptr(projection));
    unlitShader.setMatrix4("view", glm::value_ptr(view));

    glDisable(GL_DEPTH_TEST);

    const float axisLen = 1.25f;
    const float axisThick = 0.04f;
    const float handle = 0.12f;

    auto arm = [&](const glm::vec3& offset, const glm::vec3& scale, const glm::vec3& color)
        {
            glm::mat4 m(1.0f);
            m = glm::translate(m, position + offset);
            m = glm::scale(m, scale);
            unlitShader.setMatrix4("model", glm::value_ptr(m));
            unlitShader.setVector3("objectColor", color.r, color.g, color.b);
            cubeMesh.draw();
        };

    arm({ axisLen * 0.5f, 0, 0 }, { axisLen, axisThick, axisThick }, kAxisX);
    arm({ 0, axisLen * 0.5f, 0 }, { axisThick, axisLen, axisThick }, kAxisY);
    arm({ 0, 0, axisLen * 0.5f }, { axisThick, axisThick, axisLen }, kAxisZ);

    if (mode == GizmoMode::Translate)
    {
        arm({ axisLen, 0, 0 }, { handle, handle, handle }, kAxisX);
        arm({ 0, axisLen, 0 }, { handle, handle, handle }, kAxisY);
        arm({ 0, 0, axisLen }, { handle, handle, handle }, kAxisZ);
    }
    else if (mode == GizmoMode::Scale)
    {
        arm({ axisLen, 0, 0 }, { handle, handle, handle }, kAxisX);
        arm({ 0, axisLen, 0 }, { handle, handle, handle }, kAxisY);
        arm({ 0, 0, axisLen }, { handle, handle, handle }, kAxisZ);
    }
    else
    {
        arm({ axisLen, 0, 0 }, { handle * 1.3f, handle * 0.35f, handle * 1.3f }, kAxisX);
        arm({ 0, axisLen, 0 }, { handle * 1.3f, handle * 0.35f, handle * 1.3f }, kAxisY);
        arm({ 0, 0, axisLen }, { handle * 1.3f, handle * 0.35f, handle * 1.3f }, kAxisZ);
    }

    arm({ 0, 0, 0 }, { handle, handle, handle }, glm::vec3(1.0f));

    glEnable(GL_DEPTH_TEST);
}

void Renderer::drawSelectionOutline(
    const Scene& scene,
    const GameObject& object,
    const Camera& camera,
    int viewportWidth,
    int viewportHeight)
{
    if (!unlitShader.isValid())
        return;

    const float aspect =
        static_cast<float>(viewportWidth) /
        static_cast<float>(std::max(viewportHeight, 1));
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.getFieldOfView()), aspect, 0.1f, 500.0f);
    const glm::mat4 view = camera.getViewMatrix();

    unlitShader.use();
    unlitShader.setMatrix4("projection", glm::value_ptr(projection));
    unlitShader.setMatrix4("view", glm::value_ptr(view));
    unlitShader.setVector3("objectColor", 1.0f, 1.0f, 1.0f);

    // Skip UI in 3D outline path
    if (object.isUI())
        return;

    glm::mat4 model = scene.getWorldMatrix(object.getId());
    unlitShader.setMatrix4("model", glm::value_ptr(model));

    // Draw mesh as wireframe so only visible silhouette edges show from current camera
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);
    glDepthFunc(GL_LEQUAL);

    meshForObject(object).draw();

    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
}


void Renderer::drawUIOverlay(
    const Scene& scene,
    int viewportWidth,
    int viewportHeight)
{
    if (!unlitShader.isValid() || !cubeMesh.isValid())
        return;

    // Collect visible UI elements under visible UI roots
    struct Item { const GameObject* obj; };
    std::vector<Item> items;
    for (const auto& rootPtr : scene.getObjects())
    {
        const GameObject& root = *rootPtr;
        if (!root.isUIRoot() || !root.isVisible())
            continue;
        for (const auto& childPtr : scene.getObjects())
        {
            const GameObject& el = *childPtr;
            if (el.getParentId() != root.getId()) continue;
            if (!el.isUI() || el.isUIRoot()) continue;
            if (!el.isVisible()) continue;
            items.push_back({ &el });
        }
    }
    if (items.empty()) return;

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        return a.obj->getZOrder() < b.obj->getZOrder();
        });

    const float w = static_cast<float>(std::max(viewportWidth, 1));
    const float h = static_cast<float>(std::max(viewportHeight, 1));
    // Ortho: origin top-left to match UI designer coords
    glm::mat4 proj = glm::ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
    glm::mat4 view(1.0f);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unlitShader.use();
    unlitShader.setMatrix4("projection", glm::value_ptr(proj));
    unlitShader.setMatrix4("view", glm::value_ptr(view));

    for (const Item& it : items)
    {
        const GameObject& el = *it.obj;
        glm::vec2 pos(el.getPosition().x, el.getPosition().y);
        glm::vec2 size = el.getUISize();
        // Unit cube is 2x2 centered at origin → scale by size/2, translate to centre
        glm::mat4 model(1.0f);
        model = glm::translate(model, glm::vec3(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(size.x * 0.5f, size.y * 0.5f, 1.0f));
        unlitShader.setMatrix4("model", glm::value_ptr(model));
        const glm::vec3 c = el.getColor();
        unlitShader.setVector3("objectColor", c.r, c.g, c.b);
        cubeMesh.draw();
    }

    glEnable(GL_DEPTH_TEST);
}

void Renderer::render(
    const Scene& scene,
    const Camera& camera,
    int viewportWidth,
    int viewportHeight,
    std::uint64_t selectedId,
    GizmoMode gizmoMode,
    bool showGrid)
{
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return;
    if (shadowFramebuffer == 0 || shadowDepthTexture == 0)
        return;

    const glm::mat4 lightSpaceMatrix = makeLightSpaceMatrix();
    renderShadowPass(scene);

    glViewport(0, 0, viewportWidth, viewportHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspectRatio =
        static_cast<float>(viewportWidth) /
        static_cast<float>(std::max(viewportHeight, 1));

    const glm::mat4 view = camera.getViewMatrix();
    const glm::mat4 projection = glm::perspective(
        glm::radians(camera.getFieldOfView()),
        aspectRatio,
        0.1f,
        500.0f);

    shader.use();
    shader.setMatrix4("view", glm::value_ptr(view));
    shader.setMatrix4("projection", glm::value_ptr(projection));
    shader.setMatrix4("lightSpaceMatrix", glm::value_ptr(lightSpaceMatrix));
    shader.setVector3(
        "lightDirection",
        sceneLightDirection.x,
        sceneLightDirection.y,
        sceneLightDirection.z);
    shader.setVector3("lightColor", 1.0f, 0.96f, 0.90f);

    const glm::vec3 cameraPosition = camera.getPosition();
    shader.setVector3(
        "viewPosition",
        cameraPosition.x,
        cameraPosition.y,
        cameraPosition.z);

    shader.setInteger("diffuseTexture", 0);
    shader.setInteger("shadowMap", 1);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    glActiveTexture(GL_TEXTURE0);

    for (const auto& object : scene.getObjects())
        drawObject(scene, *object, shader);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    // Draw saved UI elements as 2D overlay on the engine viewport (visible only if root+element visible)
    drawUIOverlay(scene, viewportWidth, viewportHeight);

    if (showGrid)
        drawGrid(camera, viewportWidth, viewportHeight);

    if (selectedId != 0)
    {
        for (const auto& obj : scene.getObjects())
        {
            if (obj->getId() == selectedId)
            {
                drawSelectionOutline(scene, *obj, camera, viewportWidth, viewportHeight);
                drawGizmo(camera, obj->getPosition(), gizmoMode, viewportWidth, viewportHeight);
                break;
            }
        }
    }
}