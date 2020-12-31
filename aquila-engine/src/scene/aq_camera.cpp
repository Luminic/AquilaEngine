#include "scene/aq_camera.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

namespace aq {

    DefaultCamera::DefaultCamera(int render_width, int render_height, float fov) : AbstractCamera(), fov(fov) {
        render_window_size_changed(render_width, render_height);
    }

    DefaultCamera::~DefaultCamera() {}

    void DefaultCamera::update() {
        view_matrix = glm::lookAt(position, position + get_forward_vector(), glm::vec3(0.0f,-1.0f,0.0f));
    }

    void DefaultCamera::render_window_size_changed(int width, int height) {
        render_width = width;
        render_height = height;
        update_projection_matrix();
    }

    glm::vec3 DefaultCamera::get_forward_vector() {
        float yaw = glm::radians(euler_angles.x);
        float pitch = glm::radians(euler_angles.y);

        return glm::vec3(
            glm::sin(yaw) * glm::cos(pitch),
            glm::sin(pitch),
            glm::cos(yaw) * glm::cos(pitch)
        );
    }

    glm::mat4 DefaultCamera::get_view_matrix() {
        return view_matrix;
    }

    glm::mat4 DefaultCamera::get_projection_matrix() {
        return projection_matrix;
    }

    void DefaultCamera::update_projection_matrix() {
        if (render_height == 0) { // Aspect ratio of infinity (no screen to render to)
            projection_matrix = glm::mat4(1.0f);
        }
        projection_matrix = glm::perspective(glm::radians(fov), float(render_width) / render_height, 0.1f, 200.0f);
        projection_matrix[1][1] *= -1; // Flip Y
    }

}