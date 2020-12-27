#include "render_engine.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>

#include "pipeline_builder.hpp"

namespace aq {

    RenderEngine::RenderEngine() {}

    RenderEngine::~RenderEngine() {}

    void RenderEngine::update() {}

    void RenderEngine::draw(AbstractCamera* camera) {
        if (initialization_state != InitializationState::Initialized) {
            std::cerr << "`VulkanRenderEngine` can only draw when `initialization_state` is `InitializationState::Initialized`" << std::endl;
            return;
        }

        uint64_t timeout{ 1'000'000'000 };

        // Wait until the GPU has finished rendering the last frame
        CHECK_VK_RESULT( device.waitForFences(1, &render_fence, VK_TRUE, timeout), "Failed to wait for render fence");
        CHECK_VK_RESULT( device.resetFences(1, &render_fence), "Failed to reset render fence");

        // Get next swap chain image
        auto [ani_result, sw_ch_image_index] = device.acquireNextImageKHR(swap_chain, timeout, present_semaphore, {});
        if (ani_result == vk::Result::eSuboptimalKHR || ani_result == vk::Result::eErrorOutOfDateKHR) {
            if (!resize_window())
                std::cerr << "Failed to recreate swapchain when resizing window." << std::endl;
            return;
        } else {
            CHECK_VK_RESULT(ani_result, "Failed to aquire next swap chain image");
        }

        // Reset the command buffer
        CHECK_VK_RESULT(main_command_buffer.reset(), "Failed to reset main cmd buffer");

        // Begin command buffer
        vk::CommandBufferBeginInfo cmd_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        CHECK_VK_RESULT(main_command_buffer.begin(cmd_begin_info), "Failed to begin cmd buffer");

        // Set the dynamic state
        main_command_buffer.setViewport(0, {{0.0f, 0.0f, float(window_extent.width), float(window_extent.height), 0.0f, 1.0f}});
        main_command_buffer.setScissor(0, {{{0, 0}, window_extent}});

        uint64_t period = 2048;
        float flash = (frame_number%period) / float(period);
        std::array<vk::ClearValue, 1> clear_values{
            {vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,flash,0.0f}}}
        };

        vk::RenderPassBeginInfo render_pass_begin_info(
            render_pass,
            framebuffers[sw_ch_image_index],
            vk::Rect2D({0,0}, window_extent),
            clear_values
        );

        main_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        // Actual rendering
        main_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, triangle_pipeline);
        main_command_buffer.bindVertexBuffers(0, {triangle_mesh.vertex_buffer.buffer}, {0});

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(frame_number * 0.4f), glm::vec3(0, 1, 0));
        glm::mat4 mesh_matrix = camera->get_projection_matrix() * camera->get_view_matrix() * model;

        PushConstants constants;
        constants.view_projection = mesh_matrix;

        main_command_buffer.pushConstants(triangle_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &constants);
        main_command_buffer.draw(3, 1, 0, 0);


        main_command_buffer.endRenderPass();

        // End command buffer
        CHECK_VK_RESULT(main_command_buffer.end(), "Failed to end command buffer");

        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submit_info(1, &present_semaphore, &wait_stage, 1, &main_command_buffer, 1, &render_semaphore);
        CHECK_VK_RESULT(graphics_queue.submit(1, &submit_info, render_fence), "Failed to submit graphics_queue");

        vk::PresentInfoKHR present_info(1, &render_semaphore, 1, &swap_chain, &sw_ch_image_index);
        vk::Result p_result = graphics_queue.presentKHR(present_info);
        if (p_result == vk::Result::eSuboptimalKHR || p_result == vk::Result::eErrorOutOfDateKHR) {
            if (!resize_window())
                std::cerr << "Failed to recreate swapchain when resizing window." << std::endl;
        } else {
            CHECK_VK_RESULT(p_result, "Failed to present graphics queue");
        }

        ++frame_number;
    }

    glm::ivec2 RenderEngine::get_render_window_size() {
        return glm::ivec2(window_extent.width, window_extent.height);
    }

    bool RenderEngine::init_render_resources() {
        if (!init_pipelines()) return false;
        return true;
    }

    void RenderEngine::cleanup_render_resources() {
        if (device && allocator && triangle_mesh.vertex_buffer.buffer && triangle_mesh.vertex_buffer.allocation) {
            CHECK_VK_RESULT(device.waitIdle(), "Failed to wait for device to finish all tasks for cleanup");
            allocator.destroyBuffer(triangle_mesh.vertex_buffer.buffer, triangle_mesh.vertex_buffer.allocation);
            triangle_mesh.vertex_buffer.buffer = nullptr;
            triangle_mesh.vertex_buffer.allocation = nullptr;
        }

        // Destroy pipeline
        if (triangle_pipeline) device.destroyPipeline(triangle_pipeline);
        triangle_pipeline = nullptr;

        if (triangle_pipeline_layout) device.destroyPipelineLayout(triangle_pipeline_layout);
        triangle_pipeline_layout = nullptr;
    }

    bool RenderEngine::init_pipelines() {
        std::string proj_path(PROJECT_PATH);

        vk::UniqueShaderModule triangle_vert_shader = vk_init::load_shader_module_unique(
            (proj_path + "/shaders/color.vert.spv").c_str(), device
        );
        if (!triangle_vert_shader) {
            std::cerr << "Failed to load triangle_vert_shader; Aborting." << std::endl;
            return false;
        }

        vk::UniqueShaderModule triangle_frag_shader = vk_init::load_shader_module_unique(
            (proj_path + "/shaders/color.frag.spv").c_str(), device
        );
        if (!triangle_frag_shader) {
            std::cerr << "Failed to load triangle_frag_shader; Aborting." << std::endl;
            return false;
        }

        std::array<vk::PushConstantRange, 1> push_constant_ranges = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants))
        };

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, {}, push_constant_ranges);

        vk::Result cpl_result;
        std::tie(cpl_result, triangle_pipeline_layout) = device.createPipelineLayout(pipeline_layout_create_info);
        CHECK_VK_RESULT_R(cpl_result, false, "Failed to create pipeline layout");

        std::array<vk::DynamicState, 2> dynamic_states({vk::DynamicState::eViewport, vk::DynamicState::eScissor});

        VertexInputDescription vertex_input_description = Vertex::get_vertex_description();

        PipelineBuilder pipeline_builder = PipelineBuilder()
            .add_shader_stage({{}, vk::ShaderStageFlagBits::eVertex, *triangle_vert_shader, "main"})
            .add_shader_stage({{}, vk::ShaderStageFlagBits::eFragment, *triangle_frag_shader, "main"})
            .set_vertex_input({{}, vertex_input_description.bindings, vertex_input_description.attributes})
            .set_input_assembly({{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE})
            .set_viewport_count(1)
            .set_scissor_count(1)
            .set_rasterization_state({
                {}, // flags
                VK_FALSE, // depth clamp
                VK_FALSE, // rasterizer discard
                vk::PolygonMode::eFill,
                vk::CullModeFlagBits::eNone,
                vk::FrontFace::eClockwise,
                VK_FALSE, 0.0f, 0.0f, 0.0f, // depth bias info
                1.0f // line width
            })
            .add_color_blend_attachment(PipelineBuilder::default_color_blend_attachment())
            .set_multisample_state(PipelineBuilder::default_multisample_state_one_sample())
            .set_dynamic_state({{}, dynamic_states})
            .set_pipeline_layout(triangle_pipeline_layout);

        triangle_pipeline = pipeline_builder.build_pipeline(device, render_pass);
        
        if (!triangle_pipeline)
            return false;
        
        load_meshes();

        return true;
    }

    bool RenderEngine::resize_window() {
        if (!InitializationEngine::resize_window()) return false;
        return true;
    }

    void RenderEngine::load_meshes() {
        triangle_mesh.vertices.resize(3);

        triangle_mesh.vertices[0].position = { 1.0f, 1.0f, 0.0f, 1.0f};
        triangle_mesh.vertices[1].position = {-1.0f, 1.0f, 0.0f, 1.0f};
        triangle_mesh.vertices[2].position = { 0.0f,-1.0f, 0.0f, 1.0f};

        triangle_mesh.vertices[0].color = {1.0f, 1.0f, 0.0f, 0.0f};
        triangle_mesh.vertices[1].color = {1.0f, 0.0f, 1.0f, 0.0f};
        triangle_mesh.vertices[2].color = {0.0f, 0.0f, 1.0f, 0.0f};

        upload_mesh(triangle_mesh);
    }

    void RenderEngine::upload_mesh(Mesh& mesh) {
        // Allocate vertex buffer

        vk::BufferCreateInfo buffer_create_info(
            {},
            mesh.vertices.size() * sizeof(Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::SharingMode::eExclusive
        );

        vma::AllocationCreateInfo alloc_create_info({}, vma::MemoryUsage::eCpuToGpu);

        auto[cb_result, buff_alloc] = allocator.createBuffer(buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT(cb_result, "Failed to create/allocate mesh vertex-buffer");
        mesh.vertex_buffer = buff_alloc;

        // Copy vertex data into vertex buffer

        auto[mm_result, memory] = allocator.mapMemory(mesh.vertex_buffer.allocation);
        memcpy(memory, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
        allocator.unmapMemory(mesh.vertex_buffer.allocation);
    }

}