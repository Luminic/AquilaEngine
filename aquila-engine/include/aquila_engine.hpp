#ifndef AQUILA_ENGINE_HPP
#define AQUILA_ENGINE_HPP

#include <memory>
#include <vector>

#include <glm/glm.hpp>

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
        // Adds the materials to the material manager
        void upload_materials(const std::vector<std::shared_ptr<Material>>& materials);

        // Should be called for every SDL event
        // Returns `true` if the event is "consumed" (it should be ignored)
        // Example:
        // ```
        // while (SDL_PollEvent(&event) != 0) {
        //     if (aquila_engine.process_sdl_event(event)) continue;
        //     // Event handling
        // }
        // ```
        bool process_sdl_event(const SDL_Event& sdl_event);

        glm::ivec2 get_render_window_size() const {return render_engine.get_render_window_size();}
        uint64_t get_frame_number() const {return render_engine.get_frame_number();}
        SDL_Window* get_window() { return render_engine.window; }

        std::shared_ptr<Node> root_node;

    private:
        RenderEngine render_engine;
    };

}

#endif