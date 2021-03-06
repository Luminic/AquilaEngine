#ifndef AQUILA_RENDER_ENGINE_HPP
#define AQUILA_RENDER_ENGINE_HPP

#include <array>
#include <memory>

#include <glm/glm.hpp>

#include "init_engine.hpp"
#include "util/vk_types.hpp"
#include "util/vk_descriptor_set_builder.hpp"
#include "scene/aq_texture.hpp"
#include "scene/aq_material.hpp"
#include "scene/aq_mesh.hpp"
#include "scene/aq_node.hpp"
#include "scene/aq_light.hpp"
#include "scene/aq_camera.hpp"

namespace aq {

    struct PushConstants {
        glm::mat4 model;
        uint material_index;
        uint nr_lights;
    };

    struct GPUCameraData {
        glm::mat4 view_projection;
        glm::vec4 camera_position;
    };

    class RenderEngine : public InitializationEngine {
    public:
        RenderEngine(uint max_nr_textures=1);
        virtual ~RenderEngine();

        void update();
        void draw(AbstractCamera* camera, std::shared_ptr<Node> object_hierarchy);

        glm::ivec2 get_render_window_size() const {return {window_extent.width, window_extent.height};};

        uint64_t get_frame_number() const {return frame_number;}

        MaterialManager material_manager;
        LightMemoryManager light_memory_manager;

    protected:
        virtual bool init_render_resources() override;
        // If redefined in child class, either `RenderEngine::cleanup_render_resources` MUST still be called 
        // or `init_render_resources` should also be redefined in order to not leak resources
        virtual void cleanup_render_resources() override;

        bool init_pipelines();
        vk::PipelineLayout triangle_pipeline_layout;
        vk::Pipeline triangle_pipeline;

        virtual bool resize_window() override; // Calls inherited `resize_window` method from `InitializationEngine`

        bool init_data();

        unsigned char* p_cam_buff_mem;
        AllocatedBuffer camera_buffer; // Holds GPUCameraData

        bool init_descriptors();

        vk::DescriptorPool descriptor_pool;
        vk::DescriptorSetLayout global_set_layout;
        
        struct FrameData {
            vk::DescriptorSet global_descriptor;
        };
        std::array<FrameData, FRAME_OVERLAP> frame_data{};
        FrameData& get_frame_data(uint64_t frame_number) {return frame_data[frame_number%FRAME_OVERLAP];}

        bool init_imgui();

        void draw_objects(AbstractCamera* camera, const std::vector<std::pair<std::shared_ptr<Node>, glm::mat4>>& flattened_hierarchy);
        void traverse_node_hierarchy(std::shared_ptr<Node> node, NodeHierarchyTraceback traceback, std::vector<std::pair<std::shared_ptr<Node>, glm::mat4>>& flattened_hierarchy, glm::mat4 parent_transform=glm::mat4(1.0f));
        uint64_t frame_number{0};

        DescriptorSetAllocator descriptor_set_allocator;
        vk::DescriptorSetLayout per_frame_descriptor_set_layout;
        std::vector<vk::DescriptorSet> per_frame_descriptor_sets;

    private:
        uint32_t max_nr_textures;

        // Ensure meshes being rendered are alive until the rendering stops
        std::array<std::vector<std::shared_ptr<Mesh>>, FRAME_OVERLAP> meshes_in_render;

        DeletionQueue deletion_queue;

        friend class AquilaEngine;
    };

}

#endif
