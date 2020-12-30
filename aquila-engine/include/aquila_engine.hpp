#ifndef AQUILA_ENGINE_HPP
#define AQUILA_ENGINE_HPP

#include "render_engine.hpp"
#include "aq_camera.hpp"
#include "aq_node.hpp"

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

        std::shared_ptr<Node> root_node;

    private:
        RenderEngine render_engine;
    };

}

#endif