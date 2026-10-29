
#include "Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

Transform::Transform()
    : position(0.0f),
    rotationDegrees(0.0f),
    scale(1.0f)
{}

void Transform::setPosition(const glm::vec3& newPosition)
{
    position = newPosition;
}

void Transform::setRotation(const glm::vec3& newRotationDegrees)
{
    rotationDegrees = newRotationDegrees;
}

void Transform::setScale(const glm::vec3& newScale)
{
    scale = newScale;
}

glm::vec3 Transform::getPosition() const
{
    return position;
}

glm::vec3 Transform::getRotation() const
{
    return rotationDegrees;
}

glm::vec3 Transform::getScale() const
{
    return scale;
}

void Transform::translate(const glm::vec3& amount)
{
    position += amount;
}

void Transform::rotate(const glm::vec3& amountDegrees)
{
    rotationDegrees += amountDegrees;
}

glm::mat4 Transform::getLocalMatrix() const
{
    glm::mat4 matrix(1.0f);

    matrix = glm::translate(matrix, position);

    matrix = glm::rotate(
        matrix,
        glm::radians(rotationDegrees.x),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    matrix = glm::rotate(
        matrix,
        glm::radians(rotationDegrees.y),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    matrix = glm::rotate(
        matrix,
        glm::radians(rotationDegrees.z),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    matrix = glm::scale(matrix, scale);

    return matrix;
}
