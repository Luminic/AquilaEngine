#include "render_engine.hpp"

#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>

#include "util/pipeline_builder.hpp"
#include "util/vk_utility.hpp"
#include "util/vk_shaders.hpp"

namespace aq {

    RenderEngine::RenderEngine(uint max_nr_textures) : max_nr_textures(max_nr_textures) {}

    RenderEngine::~RenderEngine() {}

    void RenderEngine::update() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);

        ImGui::NewFrame();

        // Dear ImGui commands
        ImGui::ShowDemoWindow();
    }

    void RenderEngine::draw(AbstractCamera* camera, std::shared_ptr<Node> object_hierarchy) {
        if (initialization_state != InitializationState::Initialized) {
            std::cerr << "`VulkanRenderEngine` can only draw when `initialization_state` is `InitializationState::Initialized`" << std::endl;
            return;
        }

        ImGui::Render();

        std::vector<std::pair<std::shared_ptr<Node>, glm::mat4>> flattened_hierarchy;
        traverse_node_hierarchy(object_hierarchy, {}, flattened_hierarchy);

        uint64_t timeout{ 1'000'000'000 };
        uint frame_index = frame_number % FRAME_OVERLAP;
        FrameObjects& fo = get_frame_objects(frame_number);
        FrameData& fd = get_frame_data(frame_number);

        // Wait until the GPU has finished rendering the last frame
        CHECK_VK_RESULT( device.waitForFences(1, &fo.render_fence, VK_TRUE, timeout), "Failed to wait for render fence");
        CHECK_VK_RESULT( device.resetFences(1, &fo.render_fence), "Failed to reset render fence");

        // Rendering is finished so the shared pointers can be let go
        meshes_in_render[frame_index].clear();

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

        vk::RenderPassBeginInfo render_pass_begin_info = vk::RenderPassBeginInfo()
            .setRenderPass(render_pass)
            .setFramebuffer(framebuffers[sw_ch_image_index])
            .setRenderArea(vk::Rect2D({0,0}, window_extent))
            .setClearValues(clear_values);

        fo.main_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        // Actual rendering
        draw_objects(camera, flattened_hierarchy);

        // Render ImGui
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), fo.main_command_buffer);

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
        descriptor_set_allocator.init(device);
        DescriptorSetBuilder per_frame_descriptor_set_builder(&descriptor_set_allocator, device, FRAME_OVERLAP);
        material_manager.init(FRAME_OVERLAP, max_nr_textures, 50, per_frame_descriptor_set_builder, &allocator, get_default_upload_context());
        light_memory_manager.init(FRAME_OVERLAP, 25, per_frame_descriptor_set_builder, &allocator, get_default_upload_context());
        
        per_frame_descriptor_sets = per_frame_descriptor_set_builder.build();
        per_frame_descriptor_set_layout = per_frame_descriptor_set_builder.get_layout();
        deletion_queue.push_function([this](){
            descriptor_set_allocator.destroy();
            device.destroyDescriptorSetLayout(per_frame_descriptor_set_layout);
        });

        material_manager.descriptor_sets_created(per_frame_descriptor_sets);
        light_memory_manager.descriptor_sets_created(per_frame_descriptor_sets);

        if (!init_data()) return false;
        if (!init_descriptors()) return false;
        if (!init_pipelines()) return false;

        if (!init_imgui()) return false;

        return true;
    }

    void RenderEngine::cleanup_render_resources() {
        meshes_in_render.fill({}); // All frames have finished rendering
        light_memory_manager.destroy();
        material_manager.destroy();
        deletion_queue.flush();
    }

    bool RenderEngine::init_pipelines() {
        std::string proj_path(AQUILA_ENGINE_PATH);

        vk::UniqueShaderModule triangle_vert_shader = load_shader_module_unique(
            (proj_path + "/shaders/color.vert.spv").c_str(), device
        );
        if (!triangle_vert_shader) {
            std::cerr << "Failed to load triangle_vert_shader; Aborting." << std::endl;
            return false;
        }

        vk::UniqueShaderModule triangle_frag_shader = load_shader_module_unique(
            (proj_path + "/shaders/color.frag.spv").c_str(), device
        );
        if (!triangle_frag_shader) {
            std::cerr << "Failed to load triangle_frag_shader; Aborting." << std::endl;
            return false;
        }

        std::array<vk::SpecializationMapEntry, 1> triangle_frag_specialization_map{{
            {0, 0, sizeof(int32_t)}
        }};

        vk::SpecializationInfo triangle_frag_specialization = vk::SpecializationInfo()
            .setMapEntries(triangle_frag_specialization_map)
            .setDataSize(sizeof(max_nr_textures))
            .setPData(&max_nr_textures);

        std::array<vk::DescriptorSetLayout, 2> set_layouts = {{global_set_layout, per_frame_descriptor_set_layout}};

        std::array<vk::PushConstantRange, 2> push_constant_ranges = {
            vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4)),
            vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, offsetof(PushConstants, material_index), 2*sizeof(uint))
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
            .add_shader_stage({{}, vk::ShaderStageFlagBits::eFragment, *triangle_frag_shader, "main", &triangle_frag_specialization})
            .set_vertex_input({{}, vertex_input_description.bindings, vertex_input_description.attributes})
            .set_input_assembly({{}, vk::PrimitiveTopology::eTriangleList, VK_FALSE})
            .set_viewport_count(1)
            .set_scissor_count(1)
            .set_rasterization_state( vk::PipelineRasterizationStateCreateInfo()
                .setDepthClampEnable(VK_FALSE)
                .setRasterizerDiscardEnable(VK_FALSE)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setFrontFace(vk::FrontFace::eCounterClockwise)
                .setCullMode(vk::CullModeFlagBits::eNone)
                .setDepthBiasEnable(VK_FALSE)
                .setLineWidth(1.0f) )
            .add_color_blend_attachment(PipelineBuilder::default_color_blend_attachment())
            .set_multisample_state(PipelineBuilder::default_multisample_state_one_sample())
            .set_depth_stencil_state( vk::PipelineDepthStencilStateCreateInfo()
                .setDepthTestEnable(VK_TRUE)
                .setDepthWriteEnable(VK_TRUE)
                .setDepthCompareOp(vk::CompareOp::eLessOrEqual)
                .setDepthBoundsTestEnable(VK_FALSE)
                .setStencilTestEnable(VK_FALSE) )
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
        if (!camera_buffer.allocate(&allocator, camera_data_gpu_size * FRAME_OVERLAP, vk::BufferUsageFlagBits::eUniformBuffer, vma::MemoryUsage::eCpuToGpu, vk::MemoryPropertyFlagBits::eHostCoherent)) return false;
        deletion_queue.push_function([this]() { camera_buffer.destroy(); });

        auto[mm_result, buff_mem] = allocator.mapMemory(camera_buffer.allocation);
        CHECK_VK_RESULT_R(mm_result, false, "Failed to map camera buffer memory");
        p_cam_buff_mem = (unsigned char*) buff_mem;
        deletion_queue.push_function([this]() { allocator.unmapMemory(camera_buffer.allocation); });

        return true;
    }

    bool RenderEngine::init_descriptors() {

        // Descriptor Pool

        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes{{
            {vk::DescriptorType::eUniformBuffer, 10},
            {vk::DescriptorType::eSampler, 1},
            {vk::DescriptorType::eSampledImage, 1},
            {vk::DescriptorType::eCombinedImageSampler, 10}
        }};
        vk::DescriptorPoolCreateInfo desc_pool_create_info({}, 10, descriptor_pool_sizes);

        vk::Result cdp_result;
        std::tie(cdp_result, descriptor_pool) = device.createDescriptorPool(desc_pool_create_info);
        CHECK_VK_RESULT_R(cdp_result, false, "Failed to create descriptor pool");
        deletion_queue.push_function([this]() { device.destroyDescriptorPool(descriptor_pool); });

        // Descriptor Set Layouts

        // Global desc. set layout
        std::array<vk::DescriptorSetLayoutBinding, 1> buffer_bindings{{
            // Camera buffer binding
            { 0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment }
        }};
        vk::DescriptorSetLayoutCreateInfo desc_set_create_info({}, buffer_bindings);

        vk::Result cds_result;
        std::tie(cds_result, global_set_layout) = device.createDescriptorSetLayout(desc_set_create_info);
        CHECK_VK_RESULT_R(cds_result, false, "Failed to create global set layout");
        deletion_queue.push_function([this]() { device.destroyDescriptorSetLayout(global_set_layout); });

        // Descriptor Sets

        for (uint i=0; i<FRAME_OVERLAP; ++i) {

            vk::DescriptorSetAllocateInfo desc_set_alloc_info(descriptor_pool, 1, &global_set_layout);

            auto[ads_result, desc_sets] = device.allocateDescriptorSets(desc_set_alloc_info);
            CHECK_VK_RESULT_R(ads_result, false, "Failed to allocate descriptor set");
            frame_data[i].global_descriptor = desc_sets[0];

            size_t camera_data_gpu_size = vk_util::pad_uniform_buffer_size(sizeof(GPUCameraData), gpu_properties.limits.minUniformBufferOffsetAlignment);
            std::array<vk::DescriptorBufferInfo, 1> buffer_infos{{
                {camera_buffer.buffer, camera_data_gpu_size * i, camera_data_gpu_size}
            }};
            vk::WriteDescriptorSet write_desc_set = vk::WriteDescriptorSet()
                .setDstSet(frame_data[i].global_descriptor)
                .setDstBinding(0)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setBufferInfo(buffer_infos);

            device.updateDescriptorSets({write_desc_set}, {});
        }

        return true;
    }

    bool RenderEngine::init_imgui() {
        // Create the decriptor pool for Dear ImGui

        std::array<vk::DescriptorPoolSize, 11> pool_sizes{{
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        }};

        vk::Result cdp_res;
        vk::DescriptorPool imgui_desc_pool;
        std::tie(cdp_res, imgui_desc_pool) = device.createDescriptorPool({vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, pool_sizes});
        CHECK_VK_RESULT_R(cdp_res, false, "Failed to create descriptor pool for Dear ImGui");

        // Initialize Dear ImGui itself

        ImGui::CreateContext();

        // Initializes Dear ImGui for SDL
        ImGui_ImplSDL2_InitForVulkan(window);

        // Initializes Dear ImGui for Vulkan
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = chosen_gpu;
        init_info.Device = device;
        init_info.Queue = graphics_queue;
        init_info.DescriptorPool = imgui_desc_pool;
        init_info.MinImageCount = image_count;
        init_info.ImageCount = image_count;

        ImGui_ImplVulkan_Init(&init_info, render_pass);

        // Upload Dear ImGui font textures
        vk_util::immediate_submit(
            [](VkCommandBuffer cmd) {
                ImGui_ImplVulkan_CreateFontsTexture(cmd);
            },
            get_default_upload_context()
        );

        // Clear font textures from cpu data
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        // Add the destroy for Dear ImGui structures
        deletion_queue.push_function([this, imgui_desc_pool]() {
            vkDestroyDescriptorPool(device, imgui_desc_pool, nullptr);
            ImGui_ImplVulkan_Shutdown();
        });

        return true;
    }

    void RenderEngine::draw_objects(AbstractCamera* camera, const std::vector<std::pair<std::shared_ptr<Node>, glm::mat4>>& flattened_hierarchy) {
        uint frame_index = frame_number % FRAME_OVERLAP;
        FrameObjects& fo = get_frame_objects(frame_number);
        FrameData& fd = get_frame_data(frame_number);

        // Update the managers now that the frame has finished rendering
        material_manager.update(frame_index);
        uint nr_lights = (uint) light_memory_manager.update(frame_index);

        fo.main_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, triangle_pipeline);

        size_t camera_data_gpu_size = vk_util::pad_uniform_buffer_size(sizeof(GPUCameraData), gpu_properties.limits.minUniformBufferOffsetAlignment);
        GPUCameraData camera_data{
            camera->get_projection_matrix() * camera->get_view_matrix(),
            glm::vec4(camera->get_position(), 1.0f)
        };
        memcpy(p_cam_buff_mem + camera_data_gpu_size*frame_index, &camera_data, sizeof(GPUCameraData));

        fo.main_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, triangle_pipeline_layout, 0, {fd.global_descriptor}, {});
        fo.main_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, triangle_pipeline_layout, 1, {per_frame_descriptor_sets[frame_index]}, {});

        PushConstants constants;
        constants.nr_lights = nr_lights;
        fo.main_command_buffer.pushConstants(triangle_pipeline_layout, vk::ShaderStageFlagBits::eFragment, offsetof(PushConstants, nr_lights), sizeof(uint), (std::byte*)&constants + offsetof(PushConstants, nr_lights));
        
        for (auto&[node, transformation_matrix] : flattened_hierarchy) {
            if (node->get_child_meshes().size() > 0) { // Avoid pushing constants if no meshes are going to be drawn
                constants.model = transformation_matrix;
                fo.main_command_buffer.pushConstants(triangle_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(glm::mat4), &constants);

                for (auto& mesh : node->get_child_meshes()) {
                    constants.material_index = material_manager.get_material_index(mesh->material);
                    fo.main_command_buffer.pushConstants(triangle_pipeline_layout, vk::ShaderStageFlagBits::eFragment, offsetof(PushConstants, material_index), sizeof(uint), (std::byte*)&constants + offsetof(PushConstants, material_index));

                    fo.main_command_buffer.bindVertexBuffers(0, {mesh->combined_iv_buffer.buffer}, {mesh->vertex_data_offset});
                    fo.main_command_buffer.bindIndexBuffer(mesh->combined_iv_buffer.buffer, 0, index_vk_type);

                    fo.main_command_buffer.drawIndexed(mesh->indices.size(), 1, 0, 0, 0);

                    // Make sure mesh stays alive while this frame is being rendered
                    meshes_in_render[frame_index].push_back(mesh);
                }
            }
        }
    }

    void RenderEngine::traverse_node_hierarchy(std::shared_ptr<Node> node, NodeHierarchyTraceback traceback, std::vector<std::pair<std::shared_ptr<Node>, glm::mat4>>& flattened_hierarchy, glm::mat4 parent_transform) {
        node->hierarchical_update(frame_number, parent_transform);
        glm::mat4 hierarchical_transform = parent_transform * node->get_model_matrix();
        flattened_hierarchy.push_back(std::pair<std::shared_ptr<Node>&, glm::mat4>{node, hierarchical_transform});

        for (auto& child_node : node->get_child_nodes()) {
            traverse_node_hierarchy(child_node, {node, &traceback}, flattened_hierarchy, hierarchical_transform);
        }
    }

}