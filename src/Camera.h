#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float ORBIT_SPEED = 0.25f;
const float ZOOM_SPEED_RMB = 0.05f;
const float ZOOM_SPEED_SCROLL = 1.0f;
const float MIN_RADIUS = 1.0f;
const float MAX_RADIUS = 20.0f;
const float FOV = 45.0f;

class Camera {
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    glm::vec3 Target;

    // Orbital parameters
    float Radius;
    float Yaw;
    float Pitch;

    // Camera options
    float OrbitSensitivity;
    float ZoomSensitivityRMB;
    float ZoomSensitivityScroll;
    float Fov;

    Camera(
        glm::vec3 target = glm::vec3(0.0f, 0.5f, 0.0f),
        float radius = 7.0f,
        float yaw = YAW,
        float pitch = PITCH,
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix(float aspectRatio);

    void ProcessOrbit(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessZoom(float yoffset);

private:
    void updateCameraVectors();
};

#endif
