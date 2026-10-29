
#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera()
    : position(0.0f, 1.5f, 7.0f),
    front(0.0f, 0.0f, -1.0f),
    up(0.0f, 1.0f, 0.0f),
    right(1.0f, 0.0f, 0.0f),
    worldUp(0.0f, 1.0f, 0.0f),
    yaw(-90.0f),
    pitch(-10.0f),
    movementSpeed(5.0f),
    mouseSensitivity(0.1f),
    fieldOfView(45.0f)
{
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(
    const glm::vec3& direction,
    float deltaTime
)
{
    position += direction * movementSpeed * deltaTime;
}

void Camera::processMouseMovement(
    float xOffset,
    float yOffset
)
{
    yaw += xOffset * mouseSensitivity;
    pitch += yOffset * mouseSensitivity;

    pitch = std::clamp(pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void Camera::processMouseScroll(float yOffset)
{
    fieldOfView -= yOffset;
    fieldOfView = std::clamp(fieldOfView, 20.0f, 80.0f);
}

float Camera::getFieldOfView() const
{
    return fieldOfView;
}

glm::vec3 Camera::getPosition() const
{
    return position;
}

glm::vec3 Camera::getFront() const
{
    return front;
}

glm::vec3 Camera::getRight() const
{
    return right;
}

void Camera::updateCameraVectors()
{
    constexpr float degreesToRadians =
        3.14159265358979323846f / 180.0f;

    glm::vec3 direction;

    direction.x = std::cos(yaw * degreesToRadians)
        * std::cos(pitch * degreesToRadians);

    direction.y = std::sin(pitch * degreesToRadians);

    direction.z = std::sin(yaw * degreesToRadians)
        * std::cos(pitch * degreesToRadians);

    front = glm::normalize(direction);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
