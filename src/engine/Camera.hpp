
#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera();

    glm::mat4 getViewMatrix() const;

    void processKeyboard(
        const glm::vec3& direction,
        float deltaTime
    );

    void processMouseMovement(
        float xOffset,
        float yOffset
    );

    void processMouseScroll(float yOffset);

    float getFieldOfView() const;

    glm::vec3 getPosition() const;
    glm::vec3 getFront() const;
    glm::vec3 getRight() const;

private:
    void updateCameraVectors();

    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;
    float fieldOfView;
};
