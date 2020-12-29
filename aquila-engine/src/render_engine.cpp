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

    void RenderEngine::draw(AbstractCamera* camera, std::shared_ptr<Node> object_hierarchy) {
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
        std::array<vk::ClearValue, 2> clear_values{{
            {vk::ClearColorValue(std::array<float,4>{0.0f,0.0f,flash,0.0f})},
            {vk::ClearDepthStencilValue(1.0f, 0)}
        }};

        vk::RenderPassBeginInfo render_pass_begin_info(
            render_pass,
            framebuffers[sw_ch_image_index],
            vk::Rect2D({0,0}, window_extent),
            clear_values
        );

        main_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        // Actual rendering
        main_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, triangle_pipeline);

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(frame_number * 0.4f), glm::vec3(0, 1, 0));
        glm::mat4 vp = camera->get_projection_matrix() * camera->get_view_matrix();

        PushConstants constants;
        
        for (auto it=hbegin(object_hierarchy); it != hend(object_hierarchy); ++it) {
            if ((*it)->child_meshes.size() > 0) { // Avoid pushing constants if no meshes are going to be drawn
                constants.view_projection = vp * it.get_transform();
                main_command_buffer.pushConstants(triangle_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &constants);
                for (auto& mesh : (*it)->child_meshes) {
                    main_command_buffer.bindVertexBuffers(0, {mesh->vertex_buffer.buffer}, {0});
                    main_command_buffer.bindIndexBuffer(mesh->index_buffer.buffer, 0, index_vk_type);
                    // main_command_buffer.draw(mesh->vertices.size(), 1, 0, 0);
                    main_command_buffer.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
                }
            }
        }

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
        deletion_queue.flush();
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
        deletion_queue.push_function([this]() {
            if (triangle_pipeline_layout) device.destroyPipelineLayout(triangle_pipeline_layout);
            triangle_pipeline_layout = nullptr;
        });

        std::array<vk::DynamicState, 2> dynamic_states({vk::DynamicState::eViewport, vk::DynamicState::eScissor});

        Vertex::InputDescription vertex_input_description = Vertex::get_vertex_description();

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
            .set_depth_stencil_state({
                {}, // flags
                VK_TRUE, VK_TRUE, // depth test & write
                vk::CompareOp::eLessOrEqual,
                VK_FALSE, // depth bounds test
                VK_FALSE // stencil test
            })
            .set_dynamic_state({{}, dynamic_states})
            .set_pipeline_layout(triangle_pipeline_layout);

        triangle_pipeline = pipeline_builder.build_pipeline(device, render_pass);
        
        if (!triangle_pipeline)
            return false;

        deletion_queue.push_function([this]() {
            if (triangle_pipeline) device.destroyPipeline(triangle_pipeline);
            triangle_pipeline = nullptr;
        });
        
        return true;
    }

    bool RenderEngine::resize_window() {
        if (!InitializationEngine::resize_window()) return false;
        return true;
    }

}