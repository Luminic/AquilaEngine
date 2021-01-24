#ifndef SCENE_AQUILA_CAMERA_HPP
#define SCENE_AQUILA_CAMERA_HPP

#include <glm/glm.hpp>

namespace aq {

    class AbstractCamera {
    public:
        virtual ~AbstractCamera() {};

        virtual void update() {};
        virtual void render_window_size_changed(int width, int height) {};
        virtual glm::vec3 get_position() = 0;
        virtual glm::mat4 get_view_matrix() = 0;
        virtual glm::mat4 get_projection_matrix() = 0;
    };

    // Fairly standard fps camera
    class DefaultCamera : public AbstractCamera {
    public:
        DefaultCamera(int render_width=1, int render_height=1, float fov=70);
        virtual ~DefaultCamera();

        virtual void update() override;
        virtual void render_window_size_changed(int width, int height) override;

        // Returns a normalized vector pointing forwards
        virtual glm::vec3 get_forward_vector();
        virtual glm::vec3 get_position() override {return position;};
        virtual glm::mat4 get_view_matrix() override;
        virtual glm::mat4 get_projection_matrix() override;

        glm::vec3 position{};
        glm::vec3 euler_angles{}; // yaw, pitch, roll

    protected:
        void update_projection_matrix();

        int render_width, render_height;
        float fov;

        glm::mat4 view_matrix; // Created in `update`
        glm::mat4 projection_matrix;
    };

}

#endif