#include "render_engine.hpp"

#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>

#include "util/pipeline_builder.hpp"
#include "util/vk_utility.hpp"

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
        FrameObjects& fo = get_frame_objects(frame_number);
        FrameData& fd = get_frame_data(frame_number);

        // Wait until the GPU has finished rendering the last frame
        CHECK_VK_RESULT( device.waitForFences(1, &fo.render_fence, VK_TRUE, timeout), "Failed to wait for render fence");
        CHECK_VK_RESULT( device.resetFences(1, &fo.render_fence), "Failed to reset render fence");

        // Update the material manager now that the frame has finished rendering
        material_manager.update(frame_number % FRAME_OVERLAP);

        // Get next swap chain image
        auto [ani_result, sw_ch_image_index] = device.acquireNextImageKHR(swap_chain, timeout, fo.present_semaphore, {});
        if (ani_result == vk::Result::eSuboptimalKHR || ani_result == vk::Result::eErrorOutOfDateKHR) {
            if (!resize_window())
                std::cerr << "Failed to recreate swapchain when resizing window." << std::endl;
            return;
        } else {
            CHECK_VK_RESULT(ani_result, "Failed to aquire next swap chain image");
        }

        // Reset the command buffer
        CHECK_VK_RESULT(fo.main_command_buffer.reset(), "Failed to reset main cmd buffer");

        // Begin command buffer
        vk::CommandBufferBeginInfo cmd_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        CHECK_VK_RESULT(fo.main_command_buffer.begin(cmd_begin_info), "Failed to begin cmd buffer");

        // Set the dynamic state
        fo.main_command_buffer.setViewport(0, {{0.0f, 0.0f, float(window_extent.width), float(window_extent.height), 0.0f, 1.0f}});
        fo.main_command_buffer.setScissor(0, {{{0, 0}, window_extent}});

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

        fo.main_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        // Actual rendering
        draw_objects(camera, object_hierarchy);

        fo.main_command_buffer.endRenderPass();

        // End command buffer
        CHECK_VK_RESULT(fo.main_command_buffer.end(), "Failed to end command buffer");

        vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submit_info(1, &fo.present_semaphore, &wait_stage, 1, &fo.main_command_buffer, 1, &fo.render_semaphore);
        CHECK_VK_RESULT(graphics_queue.submit(1, &submit_info, fo.render_fence), "Failed to submit graphics_queue");

        vk::PresentInfoKHR present_info(1, &fo.render_semaphore, 1, &swap_chain, &sw_ch_image_index);
        vk::Result p_result = graphics_queue.presentKHR(present_info);
        if (p_result == vk::Result::eSuboptimalKHR || p_result == vk::Result::eErrorOutOfDateKHR) {
            if (!resize_window())
                std::cerr << "Failed to recreate swapchain when resizing window." << std::endl;
        } else {
            CHECK_VK_RESULT(p_result, "Failed to present graphics queue");
        }

        ++frame_number;
    }

    bool RenderEngine::init_render_resources() {
        if (!material_manager.init(100, FRAME_OVERLAP, gpu_properties.limits.minUniformBufferOffsetAlignment, &allocator, get_default_upload_context())) return false;
        if (!init_data()) return false;
        if (!init_descriptors()) return false;
        if (!init_pipelines()) return false;

        return true;
    }

    void RenderEngine::cleanup_render_resources() {
        material_manager.destroy();
        deletion_queue.flush();
    }

    bool RenderEngine::init_pipelines() {
        std::string proj_path(AQUILA_ENGINE_PATH);

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


        std::array<vk::DescriptorSetLayout, 2> set_layouts = {{global_set_layout, material_manager.get_descriptor_set_layout()}};

        std::array<vk::PushConstantRange, 1> push_constant_ranges = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants))
        };
        
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, set_layouts, push_constant_ranges);

        vk::Result cpl_result;
        std::tie(cpl_result, triangle_pipeline_layout) = device.createPipelineLayout(pipeline_layout_create_info);
        CHECK_VK_RESULT_R(cpl_result, false, "Failed to create pipeline layout");
        deletion_queue.push_function([this]() { device.destroyPipelineLayout(triangle_pipeline_layout); });

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

        deletion_queue.push_function([this]() { device.destroyPipeline(triangle_pipeline); });
        
        return true;
    }

    bool RenderEngine::resize_window() {
        if (!InitializationEngine::resize_window()) return false;
        return true;
    }

    bool RenderEngine::init_data() {

        // Camera Buffer

        size_t camera_data_gpu_size = vk_util::pad_uniform_buffer_size(sizeof(GPUCameraData), gpu_properties.limits.minUniformBufferOffsetAlignment);
        if (!camera_buffer.allocate(&allocator, camera_data_gpu_size * FRAME_OVERLAP, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu)) return false;
        deletion_queue.push_function([this]() { camera_buffer.destroy(); });

        auto[mm_result, buff_mem] = allocator.mapMemory(camera_buffer.allocation);
        CHECK_VK_RESULT_R(mm_result, false, "Failed to map camera buffer memory");
        p_cam_buff_mem = (unsigned char*) buff_mem;
        deletion_queue.push_function([this]() { allocator.unmapMemory(camera_buffer.allocation); });

        // Sampler

        vk::SamplerCreateInfo sampler_create_info(
            {}, // flags
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            0.0f, // mip lod bias
            VK_FALSE, // anisotropy enable
            0.0f // max anisotropy
        );

        vk::Result cs_result;
        std::tie(cs_result, default_sampler) = device.createSampler(sampler_create_info);
        CHECK_VK_RESULT_R(cs_result, false, "Failed to create sampler");
        deletion_queue.push_function([this]() { device.destroySampler(default_sampler); });

        if (!placeholder_texture.upload_from_file("/home/l/C++/Vulkan/Vulkan-Engine/sandbox/resources/happy-tree.png", &allocator, get_default_upload_context())) return false;
        deletion_queue.push_function([this]() { placeholder_texture.destroy(); });

        return true;
    }

    bool RenderEngine::init_descriptors() {

        // Descriptor Pool

        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes{{
            {vk::DescriptorType::eUniformBuffer, 10},
            {vk::DescriptorType::eUniformBufferDynamic, 10},
            {vk::DescriptorType::eCombinedImageSampler, 10}
        }};
        vk::DescriptorPoolCreateInfo desc_pool_create_info({}, 10, descriptor_pool_sizes);

        // Descriptor Set Layouts

        // Global desc. set layout
        std::array<vk::DescriptorSetLayoutBinding, 1> buffer_bindings{{
            // Camera buffer binding
            { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex }
        }};
        vk::DescriptorSetLayoutCreateInfo desc_set_create_info({}, buffer_bindings);

        vk::Result cds_result;
        std::tie(cds_result, global_set_layout) = device.createDescriptorSetLayout(desc_set_create_info);
        CHECK_VK_RESULT_R(cds_result, false, "Failed to create global set layout");
        deletion_queue.push_function([this]() { device.destroyDescriptorSetLayout(global_set_layout); });

        // Texture desc. set layout
        /*
        std::array<vk::DescriptorSetLayoutBinding, 1> texture_bindings{{
            { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment }
        }};
        vk::DescriptorSetLayoutCreateInfo tex_desc_set_create_info({}, texture_bindings);

        vk::Result ctds_res;
        std::tie(ctds_res, texture_set_layout) = device.createDescriptorSetLayout(tex_desc_set_create_info);
        CHECK_VK_RESULT_R(ctds_res, false, "Failed to create texture set layout");
        deletion_queue.push_function([this]() { device.destroyDescriptorSetLayout(texture_set_layout); });
        */

        // Descriptor Sets

        vk::Result cdp_result;
        std::tie(cdp_result, descriptor_pool) = device.createDescriptorPool(desc_pool_create_info);
        CHECK_VK_RESULT_R(cdp_result, false, "Failed to create descriptor pool");
        deletion_queue.push_function([this]() { device.destroyDescriptorPool(descriptor_pool); });

        for (uint i=0; i<FRAME_OVERLAP; ++i) {

            vk::DescriptorSetAllocateInfo desc_set_alloc_info(descriptor_pool, 1, &global_set_layout);

            auto[ads_result, desc_sets] = device.allocateDescriptorSets(desc_set_alloc_info);
            CHECK_VK_RESULT_R(ads_result, false, "Failed to allocate descriptor set");
            frame_data[i].global_descriptor = desc_sets[0];

            size_t camera_data_gpu_size = vk_util::pad_uniform_buffer_size(sizeof(GPUCameraData), gpu_properties.limits.minUniformBufferOffsetAlignment);
            std::array<vk::DescriptorBufferInfo, 1> buffer_infos{{
                {camera_buffer.buffer, camera_data_gpu_size * i, camera_data_gpu_size}
            }};
            vk::WriteDescriptorSet write_desc_set(
                frame_data[i].global_descriptor, // dst set
                0, // dst binding
                0, // dst array element
                vk::DescriptorType::eUniformBuffer,
                {}, // image infos
                buffer_infos
            );
            device.updateDescriptorSets({write_desc_set}, {});
        }

        material_manager.create_descriptor_set(descriptor_pool);

        // Texture descriptor set
        /*
        vk::DescriptorSetAllocateInfo tex_desc_set_alloc_info(descriptor_pool, 1, &texture_set_layout);

        auto[atds_res, tex_desc_sets] = device.allocateDescriptorSets(tex_desc_set_alloc_info);
        CHECK_VK_RESULT_R(atds_res, false, "Failed to allocate descriptor set");
        default_texture_descriptor = tex_desc_sets[0];

        std::array<vk::DescriptorImageInfo, 1> image_infos{{
            placeholder_texture.get_image_info(default_sampler)
        }};
        vk::WriteDescriptorSet write_tex_desc_set(
            default_texture_descriptor, // dst set
            0, // dst binding
            0, // dst array element
            vk::DescriptorType::eCombinedImageSampler,
            image_infos, // image infos
            {} // buffer infos
        );
        device.updateDescriptorSets({write_tex_desc_set}, {});
        */

        return true;
    }

    void RenderEngine::draw_objects(AbstractCamera* camera, std::shared_ptr<Node>& object_hierarchy) {
        uint frame_index = frame_number % FRAME_OVERLAP;
        FrameObjects& fo = get_frame_objects(frame_number);
        FrameData& fd = get_frame_data(frame_number);

        fo.main_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, triangle_pipeline);

        size_t camera_data_gpu_size = vk_util::pad_uniform_buffer_size(sizeof(GPUCameraData), gpu_properties.limits.minUniformBufferOffsetAlignment);
        GPUCameraData camera_data{camera->get_projection_matrix() * camera->get_view_matrix()};
        memcpy(p_cam_buff_mem + camera_data_gpu_size*frame_index, &camera_data, sizeof(GPUCameraData));

        fo.main_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, triangle_pipeline_layout, 0, {fd.global_descriptor}, {});

        PushConstants constants;
        
        uint i=0;
        for (auto it=hbegin(object_hierarchy); it != hend(object_hierarchy); ++it) {
            if ((*it)->get_child_meshes().size() > 0) { // Avoid pushing constants if no meshes are going to be drawn
                constants.model = it.get_transform();
                fo.main_command_buffer.pushConstants(triangle_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &constants);
                for (auto& mesh : (*it)->get_child_meshes()) {
                    uint offset = (uint32_t) material_manager.get_buffer_offset(frame_index);
                    offset += ((i++)%2) * vk_util::pad_uniform_buffer_size(sizeof(Material::Properties), gpu_properties.limits.minUniformBufferOffsetAlignment);
                    // std::cout << ((i)%2) << '\n';

                    fo.main_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, triangle_pipeline_layout, 1, {material_manager.get_descriptor_set()}, {offset});

                    fo.main_command_buffer.bindVertexBuffers(0, {mesh->combined_iv_buffer.buffer}, {mesh->vertex_data_offset});
                    fo.main_command_buffer.bindIndexBuffer(mesh->combined_iv_buffer.buffer, 0, index_vk_type);

                    fo.main_command_buffer.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);
                }
            }
        }
    }

}