#include "Picker.hpp"
#include "GameObject.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    struct Ray
    {
        glm::vec3 origin{ 0.0f };
        glm::vec3 dir{ 0.0f, 0.0f, -1.0f };
    };

    Ray screenToRay(
        const Camera& camera,
        int mouseX,
        int mouseY,
        int width,
        int height)
    {
        const float w = static_cast<float>(std::max(width, 1));
        const float h = static_cast<float>(std::max(height, 1));

        const float ndcX = (2.0f * static_cast<float>(mouseX)) / w - 1.0f;
        const float ndcY = 1.0f - (2.0f * static_cast<float>(mouseY)) / h;

        const float aspect = w / h;
        const glm::mat4 projection = glm::perspective(
            glm::radians(camera.getFieldOfView()),
            aspect,
            0.1f,
            100.0f
        );

        const glm::mat4 invView = glm::inverse(camera.getViewMatrix());
        const glm::mat4 invProj = glm::inverse(projection);

        glm::vec4 clip(ndcX, ndcY, -1.0f, 1.0f);
        glm::vec4 eye = invProj * clip;
        eye = glm::vec4(eye.x, eye.y, -1.0f, 0.0f);

        glm::vec3 worldDir = glm::normalize(glm::vec3(invView * eye));
        glm::vec3 origin = camera.getPosition();

        return { origin, worldDir };
    }

    bool rayAabb(
        const Ray& ray,
        const glm::vec3& minB,
        const glm::vec3& maxB,
        float& tHit)
    {
        float tmin = 0.0f;
        float tmax = 100000.0f;

        for (int i = 0; i < 3; ++i)
        {
            const float o = ray.origin[i];
            const float d = ray.dir[i];
            const float mn = minB[i];
            const float mx = maxB[i];

            if (std::abs(d) < 1e-8f)
            {
                if (o < mn || o > mx)
                    return false;
            }
            else
            {
                float t1 = (mn - o) / d;
                float t2 = (mx - o) / d;
                if (t1 > t2)
                    std::swap(t1, t2);

                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);

                if (tmin > tmax)
                    return false;
            }
        }

        tHit = tmin;
        return tmax >= 0.0f;
    }
}

std::uint64_t Picker::pickObject(
    const Scene& scene,
    const Camera& camera,
    int mouseX,
    int mouseY,
    int viewportWidth,
    int viewportHeight)
{
    if (viewportWidth <= 0 || viewportHeight <= 0)
        return 0;

    const Ray ray = screenToRay(camera, mouseX, mouseY, viewportWidth, viewportHeight);

    float bestT = std::numeric_limits<float>::max();
    std::uint64_t bestId = 0;

    for (const auto& obj : scene.getObjects())
    {
        const glm::vec3 pos = obj->getPosition();
        const glm::vec3 scale = obj->getScale();

        // Matches unit cube roughly in [-1,1] scaled by object scale
        glm::vec3 half = scale;
        if (obj->getMeshType() == MeshType::Plane)
            half = glm::vec3(scale.x * 0.5f, 0.05f, scale.z * 0.5f);

        const glm::vec3 minB = pos - half;
        const glm::vec3 maxB = pos + half;

        float t = 0.0f;
        if (rayAabb(ray, minB, maxB, t) && t >= 0.0f && t < bestT)
        {
            bestT = t;
            bestId = obj->getId();
        }
    }

    return bestId;
}