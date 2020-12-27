#ifndef VK_RENDER_ENGINE_HPP
#define VK_RENDER_ENGINE_HPP

#include "init_engine.hpp"
#include "aq_camera.hpp"
#include "vk_mesh.hpp"

namespace aq {

    struct PushConstants {
        glm::vec4 data;
        glm::mat4 view_projection;
    };


    class RenderEngine : public InitializationEngine {
    public:
        RenderEngine();
        virtual ~RenderEngine();

        void update();
        void draw(AbstractCamera* camera);

        glm::ivec2 get_render_window_size();

    protected:
        virtual bool init_render_resources() override;
        virtual void cleanup_render_resources() override;

        bool init_pipelines();
        vk::PipelineLayout triangle_pipeline_layout;
        vk::Pipeline triangle_pipeline;

        virtual bool resize_window() override; // Calls inherited `resize_window` method from `InitializationEngine`

        // Rendering code

        void load_meshes();
        Mesh triangle_mesh;

        void upload_mesh(Mesh& mesh);

        // Misc resources

        uint64_t frame_number{0};
    };

}

#endif
