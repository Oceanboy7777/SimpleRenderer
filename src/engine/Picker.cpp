#include "Picker.hpp"
#include "GameObject.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    struct Ray { glm::vec3 origin; glm::vec3 dir; };

    Ray screenToRay(const Camera& camera, int mouseX, int mouseY, int width, int height)
    {
        const float w = static_cast<float>((std::max)(width, 1));
        const float h = static_cast<float>((std::max)(height, 1));
        const float ndcX = (2.0f * mouseX) / w - 1.0f;
        const float ndcY = 1.0f - (2.0f * mouseY) / h;
        const float aspect = w / h;
        const glm::mat4 projection = glm::perspective(
            glm::radians(camera.getFieldOfView()), aspect, 0.1f, 500.0f);
        const glm::mat4 invView = glm::inverse(camera.getViewMatrix());
        const glm::mat4 invProj = glm::inverse(projection);
        glm::vec4 eye = invProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
        eye = glm::vec4(eye.x, eye.y, -1.0f, 0.0f);
        return { camera.getPosition(), glm::normalize(glm::vec3(invView * eye)) };
    }

    bool rayAabb(const Ray& ray, const glm::vec3& minB, const glm::vec3& maxB, float& tHit)
    {
        float tmin = 0.0f;
        float tmax = 1e6f;
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
                if (t1 > t2) std::swap(t1, t2);
                tmin = (std::max)(tmin, t1);
                tmax = (std::min)(tmax, t2);
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
        // UI elements are not world-selectable
        if (obj->isUI())
            continue;

        const glm::vec3 pos = obj->getPosition();
        const glm::vec3 scale = obj->getScale();

        // Generous world AABB so distant objects remain selectable
        glm::vec3 half;
        if (obj->getMeshType() == MeshType::Plane)
        {
            half = glm::vec3((std::max)(scale.x * 0.5f, 0.5f), 0.25f, (std::max)(scale.z * 0.5f, 0.5f));
        }
        else if (!obj->getMeshPath().empty() || obj->getMeshType() == MeshType::ImportedModel)
        {
            half = glm::abs(scale) * 2.0f;
            half = glm::max(half, glm::vec3(0.5f));
        }
        else
        {
            // Unit cube in [-1,1] → half extents = scale; min size for pickability
            half = glm::max(glm::abs(scale), glm::vec3(0.35f));
        }

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