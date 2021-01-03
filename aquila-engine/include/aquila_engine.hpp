#ifndef AQUILA_ENGINE_HPP
#define AQUILA_ENGINE_HPP

#include <memory>

#include "render_engine.hpp"
#include "scene/aq_camera.hpp"
#include "scene/aq_node.hpp"
#include "scene/aq_material.hpp"

namespace aq {

    class AquilaEngine {
    public:
        AquilaEngine();
        ~AquilaEngine();

        void update();
        void draw(AbstractCamera* camera);

        // Upload the vertices in each mesh in the `root_node` hierarchy
        // into the gpu for later draw calls.
        // Does nothing to meshes that already have been uploaded
        void upload_meshes();

        glm::ivec2 get_render_window_size() const {return render_engine.get_render_window_size();}
        uint64_t get_frame_number() const {return render_engine.get_frame_number();}
        SDL_Window* get_window() { return render_engine.window; }

        std::shared_ptr<Node> root_node;

    private:
        RenderEngine render_engine;
        std::shared_ptr<Material> placeholder_material;
        std::shared_ptr<Material> placeholder_material2;
    };

}

#endif