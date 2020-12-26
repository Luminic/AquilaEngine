#ifndef VK_RENDER_ENGINE_HPP
#define VK_RENDER_ENGINE_HPP

#include "vk_init_engine.hpp"
#include "camera.hpp"
#include "vk_mesh.hpp"


struct PushConstants {
    glm::vec4 data;
    glm::mat4 view_projection;
};


class VulkanRenderEngine : public VulkanInitializationEngine {
public:
    VulkanRenderEngine();
    virtual ~VulkanRenderEngine();

    void run();

protected:
    virtual bool init_render_resources() override;
    virtual void cleanup_render_resources() override;

    bool init_pipelines();
    vk::PipelineLayout triangle_pipeline_layout;
    vk::Pipeline triangle_pipeline;

    AbstractCamera* p_camera = nullptr;
    DefaultCamera default_camera;

    virtual bool resize_window() override; // Calls inherited `resize_window` method from `VulkanInitializationEngine`

    // Rendering code

    void load_meshes();
    Mesh triangle_mesh;

    void upload_mesh(Mesh& mesh);

    // Actual draw call

    void update();
    void draw();
    uint64_t frame_number{0};
};

#endif
