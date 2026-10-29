
#include "Material.hpp"


Material::Material(
    const std::string& name,
    const std::string& texturePath,
    const glm::vec3& tint
)
    : name(name),
    texture(texturePath, true),
    tint(tint)
{
}

const std::string& Material::getName() const
{
    return name;
}

glm::vec3 Material::getTint() const
{
    return tint;
}

void Material::setTint(const glm::vec3& newTint)
{
    tint = newTint;
}

void Material::bind() const
{
    texture.bind(0);
}

bool Material::isValid() const
{
    return texture.isValid();
}

bool Material::usesFileTexture() const
{
    return texture.wasLoadedFromFile();
}
