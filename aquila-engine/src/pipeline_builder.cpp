#include "pipeline_builder.hpp"

namespace aq {

    PipelineBuilder& PipelineBuilder::set_shader_stages(const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages) {
        this->shader_stages = shader_stages;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_shader_stage(const vk::PipelineShaderStageCreateInfo& shader_stage) {
        shader_stages.push_back(shader_stage);
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_vertex_input(const vk::PipelineVertexInputStateCreateInfo& vertex_input) {
        this->vertex_input = vertex_input;
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_input_assembly(const vk::PipelineInputAssemblyStateCreateInfo& input_assembly) {
        this->input_assembly = input_assembly;
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_viewports(const std::vector<vk::Viewport>& viewports) {
        this->viewports = viewports;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_viewport(const vk::Viewport& viewport) {
        viewports.push_back(viewport);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_viewport_count(uint32_t viewport_count) {
        viewports.resize(viewport_count);
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_scissors(const std::vector<vk::Rect2D>& scissors) {
        this->scissors = scissors;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_scissor(const vk::Rect2D& scissor) {
        scissors.push_back(scissor);
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_scissor_count(uint32_t scissor_count) {
        scissors.resize(scissor_count);
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_rasterization_state(const vk::PipelineRasterizationStateCreateInfo& rasterization_state) {
        this->rasterization_state = rasterization_state;
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_color_blend_attachments(const std::vector<vk::PipelineColorBlendAttachmentState>& color_blend_attachments) {
        this->color_blend_attachments = color_blend_attachments;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::add_color_blend_attachment(const vk::PipelineColorBlendAttachmentState& color_blend_attachment) {
        color_blend_attachments.push_back(color_blend_attachment);
        return *this;
    }

    vk::PipelineColorBlendAttachmentState PipelineBuilder::default_color_blend_attachment() {
        return vk::PipelineColorBlendAttachmentState(
            VK_FALSE, // blend enable
            vk::BlendFactor::eZero, // src color blend factor
            vk::BlendFactor::eZero, // dst color blend factor
            vk::BlendOp::eAdd, // color blend op
            vk::BlendFactor::eZero, // src alpha blend factor
            vk::BlendFactor::eZero, // dst alpha blend factor
            vk::BlendOp::eAdd, // alpha blend op
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA // color write mask
        );
    }


    PipelineBuilder& PipelineBuilder::set_multisample_state(const vk::PipelineMultisampleStateCreateInfo& multisample_state) {
        this->multisample_state = multisample_state;
        return *this;
    }

    vk::PipelineMultisampleStateCreateInfo PipelineBuilder::default_multisample_state_one_sample() {
        return vk::PipelineMultisampleStateCreateInfo(
            {}, // flags
            vk::SampleCountFlagBits::e1,
            VK_FALSE, // sample shading enable
            1.0f, // min sample shading
            nullptr, // sample mask
            VK_FALSE, // alpha to coverage enable
            VK_FALSE // alpha to one enable
        );
    }


    PipelineBuilder& PipelineBuilder::set_depth_stencil_state(const vk::PipelineDepthStencilStateCreateInfo& depth_stencil_state) {
        this->depth_stencil_state = depth_stencil_state;
        return *this;
    }


    PipelineBuilder& PipelineBuilder::set_dynamic_state(const vk::PipelineDynamicStateCreateInfo& dynamic_state) {
        this->dynamic_state = dynamic_state;
        return *this;
    }

    PipelineBuilder& PipelineBuilder::set_pipeline_layout(const vk::PipelineLayout& pipeline_layout) {
        this->pipeline_layout = pipeline_layout;
        return *this;
    }


    vk::Pipeline PipelineBuilder::build_pipeline(vk::Device device, vk::RenderPass render_pass) {
        vk::PipelineViewportStateCreateInfo viewport_state({}, viewports, scissors);

        vk::PipelineColorBlendStateCreateInfo color_blending(
            {}, // flags
            VK_FALSE, // logic op enable
            vk::LogicOp::eCopy,
            color_blend_attachments
        );

        vk::GraphicsPipelineCreateInfo pipeline_create_info(
            {}, // flags
            shader_stages, 
            &vertex_input, 
            &input_assembly, 
            nullptr, // tessellation state
            &viewport_state,
            &rasterization_state,
            &multisample_state,
            &depth_stencil_state,
            &color_blending,
            &dynamic_state,
            pipeline_layout,
            render_pass,
            0, // subpass
            nullptr, // base pipeline handle
            0 // base pipeline index
        );

        auto [cgp_result, pipeline] = device.createGraphicsPipeline(nullptr, pipeline_create_info);
        CHECK_VK_RESULT_R(cgp_result, nullptr, "Failed to build pipeline");

        return pipeline;
    }

}