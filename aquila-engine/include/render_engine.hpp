#ifndef AQUILA_RENDER_ENGINE_HPP
#define AQUILA_RENDER_ENGINE_HPP

#include "init_engine.hpp"
#include "aq_camera.hpp"
#include "aq_mesh.hpp"
#include "aq_node.hpp"

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
        void draw(AbstractCamera* camera, std::shared_ptr<Node> object_hierarchy);

        glm::ivec2 get_render_window_size() const {return {window_extent.width, window_extent.height};};
        uint64_t get_frame_number() const {return frame_number;}

    protected:
        virtual bool init_render_resources() override;
        void cleanup_render_resources() final; // Cannot be overriden or the resources would leak

        bool init_pipelines();
        vk::PipelineLayout triangle_pipeline_layout;
        vk::Pipeline triangle_pipeline;

        virtual bool resize_window() override; // Calls inherited `resize_window` method from `InitializationEngine`

        // Misc resources

        uint64_t frame_number{0};

        friend class AquilaEngine;
    
    private:
        DeletionQueue deletion_queue;
    };

}

#endif
