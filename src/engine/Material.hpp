
#pragma once

#include "Texture.hpp"

#include <glm/glm.hpp>

#include <string>

class Material
{
public:
    Material(
        const std::string& name,
        const std::string& texturePath,
        const glm::vec3& tint
    );

    const std::string& getName() const;
    glm::vec3 getTint() const;

    void setTint(const glm::vec3& tint);

    void bind() const;

    bool isValid() const;
    bool usesFileTexture() const;

private:
    std::string name;
    Texture texture;
    glm::vec3 tint;
};
