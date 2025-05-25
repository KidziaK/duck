#include "Camera.h"

Camera::Camera(glm::vec3 target, float radius, float yaw, float pitch, glm::vec3 up)
    : Target(target),
      Radius(radius),
      Yaw(yaw),
      Pitch(pitch),
      WorldUp(up),
      OrbitSensitivity(ORBIT_SPEED),
      ZoomSensitivityRMB(ZOOM_SPEED_RMB),
      ZoomSensitivityScroll(ZOOM_SPEED_SCROLL),
      Fov(FOV) {
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Target, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) {
    return glm::perspective(glm::radians(Fov), aspectRatio, 0.1f, 100.0f);
}

void Camera::ProcessOrbit(float xoffset, float yoffset, bool constrainPitch) {
    Yaw += xoffset * OrbitSensitivity;
    Pitch += yoffset * OrbitSensitivity;

    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessZoom(float yoffset) {
    Radius -= yoffset;

    if (Radius < MIN_RADIUS) Radius = MIN_RADIUS;
    if (Radius > MAX_RADIUS) Radius = MAX_RADIUS;

    updateCameraVectors();
}


void Camera::updateCameraVectors() {
    Position.x = Target.x + Radius * cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));
    Position.y = Target.y + Radius * sin(glm::radians(Pitch));
    Position.z = Target.z + Radius * cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));

    Front = glm::normalize(Target - Position);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}
