#ifndef PLAYERCOMPONENT_H
#define PLAYERCOMPONENT_H

#include <glm/glm.hpp>

struct PlayerComponent {
    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float jumpForce = 8.0f;
    bool isGrounded = true;
    bool canJump = true;
    int killCount = 0; // Track bot kills
    
    // Health tracking for ground detection
    float groundCheckDistance = 0.1f;

    // Camera settings for first-person
    float cameraHeight = 1.8f; // Height above player position
    glm::vec3 cameraOffset{0.0f, 1.8f, 0.0f};
};

#endif // PLAYERCOMPONENT_H