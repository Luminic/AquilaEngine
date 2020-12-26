#ifndef VK_RENDER_ENGINE_HPP
#define VK_RENDER_ENGINE_HPP

#include "vk_init_engine.hpp"
#include "vk_mesh.hpp"


struct PushConstants {
    glm::vec4 data;
    glm::mat4 view_projection;
};


class VulkanRenderEngine : public VulkanInitializationEngine {
public:
    VulkanRenderEngine();
    ~VulkanRenderEngine();

    void run();

protected:
    virtual bool init_render_resources();
    virtual void cleanup_render_resources();

    bool init_pipelines();
    vk::PipelineLayout triangle_pipeline_layout;
    vk::Pipeline triangle_pipeline;

    // Rendering code

    void load_meshes();
    Mesh triangle_mesh;

    void upload_mesh(Mesh& mesh);

    // Actual draw call

    void draw();
    uint64_t frame_number{0};
};

#endif
