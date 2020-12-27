#include "camera_controller.hpp"

#include <iostream>

CameraController::CameraController(aq::DefaultCamera* camera) : camera(camera) {}

void CameraController::move(CameraMovementFlags movement_flags) {
    glm::vec3 movement(0.0f);
    glm::vec3 camera_front = camera->get_forward_vector();
    camera_front.y = 0.0f;
    camera_front = normalize(camera_front);
    glm::vec3 camera_up(0.0f, -1.0f, 0.0f); // For now up will always point up
    glm::vec3 camera_right = glm::normalize(glm::cross(camera_front, camera_up));

    if ((movement_flags & CameraMovementFlags::FORWARD) != CameraMovementFlags::NONE) {
        movement += camera_front;
    }
    if ((movement_flags & CameraMovementFlags::BACKWARD) != CameraMovementFlags::NONE) {
        movement -= camera_front;
    }
    if ((movement_flags & CameraMovementFlags::RIGHTWARD) != CameraMovementFlags::NONE) {
        movement += camera_right;
    }
    if ((movement_flags & CameraMovementFlags::LEFTWARD) != CameraMovementFlags::NONE) {
        movement -= camera_right;
    }
    if ((movement_flags & CameraMovementFlags::UPWARD) != CameraMovementFlags::NONE) {
        movement += camera_up;
    }
    if ((movement_flags & CameraMovementFlags::DOWNWARD) != CameraMovementFlags::NONE) {
        movement -= camera_up;
    }

    if (glm::length(movement) != 0) {
        movement = glm::normalize(movement) * 0.01f;
        camera->position += movement;
    }
}

void CameraController::rotate(int dx, int dy) {
    camera->euler_angles.x += dx * 0.15f;
    camera->euler_angles.y += dy * 0.15f;
    if (camera->euler_angles.y > 89.0f) camera->euler_angles.y = 89.0f;
    if (camera->euler_angles.y <-89.0f) camera->euler_angles.y =-89.0f;

}