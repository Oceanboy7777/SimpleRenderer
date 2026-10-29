
#pragma once

#include <glm/glm.hpp>

class Transform
{
public:
    Transform();

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotationDegrees);
    void setScale(const glm::vec3& scale);

    glm::vec3 getPosition() const;
    glm::vec3 getRotation() const;
    glm::vec3 getScale() const;

    void translate(const glm::vec3& amount);
    void rotate(const glm::vec3& amountDegrees);

    glm::mat4 getLocalMatrix() const;

private:
    glm::vec3 position;
    glm::vec3 rotationDegrees;
    glm::vec3 scale;
};
