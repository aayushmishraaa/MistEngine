#include "Camera.h"

// Constructor
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f), Zoom(45.0f) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

// Returns the view matrix
glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

// Returns the projection matrix
glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 500.0f);
}

// Processes keyboard input
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += WorldUp * velocity;
    if (direction == DOWN)
        Position -= WorldUp * velocity;
}

// Processes mouse movement
void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // Constrain pitch to avoid flipping
    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

// Processes mouse scroll
void Camera::ProcessMouseScroll(float yoffset) {
    if (OrbitMode) {
        ZoomTowardFocal(yoffset);
        return;
    }
    Zoom -= yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::SetOrbitMode(bool enabled) {
    OrbitMode = enabled;
    if (enabled) {
        // Calculate focal point from current position + front * distance
        FocalPoint = Position + Front * OrbitDistance;
    }
}

void Camera::OrbitAround(float yawDelta, float pitchDelta) {
    Yaw += yawDelta;
    Pitch += pitchDelta;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

    updateCameraVectors();

    // Reposition camera at OrbitDistance behind the focal point
    Position = FocalPoint - Front * OrbitDistance;
}

void Camera::Pan(float rightDelta, float upDelta) {
    glm::vec3 offset = Right * rightDelta + Up * upDelta;
    Position += offset;
    FocalPoint += offset;
}

void Camera::ZoomTowardFocal(float delta) {
    OrbitDistance -= delta;
    if (OrbitDistance < 0.5f) OrbitDistance = 0.5f;
    if (OrbitDistance > 200.0f) OrbitDistance = 200.0f;

    Position = FocalPoint - Front * OrbitDistance;
}

void Camera::FocusOn(const glm::vec3& target, float distance) {
    FocalPoint = target;
    OrbitDistance = distance;
    OrbitMode = true;

    // Keep current view direction, just reposition
    Position = FocalPoint - Front * OrbitDistance;
}

// Updates camera vectors based on Euler angles
void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // Recalculate Right and Up vectors
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}