#ifndef UTIL_AQUILA_PIPELINE_BUILDER_HPP
#define UTIL_AQUILA_PIPELINE_BUILDER_HPP

#include <vector>
#include <optional>

#include "util/vk_types.hpp"

namespace aq {

    class PipelineBuilder {
    public:
        PipelineBuilder& set_shader_stages(const std::vector<vk::PipelineShaderStageCreateInfo>& shader_stages);
        PipelineBuilder& add_shader_stage(const vk::PipelineShaderStageCreateInfo& shader_stage);

        PipelineBuilder& set_vertex_input(const vk::PipelineVertexInputStateCreateInfo& vertex_input);

        PipelineBuilder& set_input_assembly(const vk::PipelineInputAssemblyStateCreateInfo& input_assembly);

        PipelineBuilder& set_viewports(const std::vector<vk::Viewport>& viewports);
        PipelineBuilder& add_viewport(const vk::Viewport& viewport);
        // For use with dynamic state
        PipelineBuilder& set_viewport_count(uint32_t viewport_count);

        PipelineBuilder& set_scissors(const std::vector<vk::Rect2D>& scissors);
        PipelineBuilder& add_scissor(const vk::Rect2D& scissor);
        // For use with dynamic state
        PipelineBuilder& set_scissor_count(uint32_t scissor_count);

        PipelineBuilder& set_rasterization_state(const vk::PipelineRasterizationStateCreateInfo& rasterization_state);

        PipelineBuilder& set_color_blend_attachments(const std::vector<vk::PipelineColorBlendAttachmentState>& color_blend_attachments);
        PipelineBuilder& add_color_blend_attachment(const vk::PipelineColorBlendAttachmentState& color_blend_attachment);
        static vk::PipelineColorBlendAttachmentState default_color_blend_attachment();

        PipelineBuilder& set_multisample_state(const vk::PipelineMultisampleStateCreateInfo& multisample_state);
        static vk::PipelineMultisampleStateCreateInfo default_multisample_state_one_sample();

        PipelineBuilder& set_depth_stencil_state(const vk::PipelineDepthStencilStateCreateInfo& depth_stencil_state);

        PipelineBuilder& set_dynamic_state(const vk::PipelineDynamicStateCreateInfo& dynamic_state);

        PipelineBuilder& set_pipeline_layout(const vk::PipelineLayout& pipeline_layout);

        
        vk::Pipeline build_pipeline(vk::Device device, vk::RenderPass render_pass);

    private:
        std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
        std::optional<vk::PipelineVertexInputStateCreateInfo> vertex_input;
        std::optional<vk::PipelineInputAssemblyStateCreateInfo> input_assembly;
        std::vector<vk::Viewport> viewports;
        std::vector<vk::Rect2D> scissors;
        std::optional<vk::PipelineRasterizationStateCreateInfo> rasterization_state;
        std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;
        std::optional<vk::PipelineMultisampleStateCreateInfo> multisample_state;
        std::optional<vk::PipelineDepthStencilStateCreateInfo> depth_stencil_state;
        std::optional<vk::PipelineDynamicStateCreateInfo> dynamic_state;
        std::optional<vk::PipelineLayout> pipeline_layout;
    };

}

#endif