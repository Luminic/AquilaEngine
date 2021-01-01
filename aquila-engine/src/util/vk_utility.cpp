#include "util/vk_utility.hpp"

namespace aq {
namespace vk_util {

    bool immediate_submit(std::function<void(vk::CommandBuffer)>&& function, const UploadContext& ctx) {
        vk::CommandBufferAllocateInfo cmd_buff_alloc_info(ctx.command_pool, vk::CommandBufferLevel::ePrimary, 1);
        
        auto[acb_result, cmd_buffs] = ctx.device.allocateCommandBuffers(cmd_buff_alloc_info);
        CHECK_VK_RESULT_R(acb_result, false, "Failed to allocate command buffer for an immediate submit");
        vk::CommandBuffer& cmd_buff = cmd_buffs[0];

        vk::CommandBufferBeginInfo cmd_buff_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
        CHECK_VK_RESULT_R(cmd_buff.begin(cmd_buff_begin_info), false, "Failed to begin command buffer");

        function(cmd_buff);

        CHECK_VK_RESULT_R(cmd_buff.end(), false, "Failed to end command buffer");

        vk::SubmitInfo submit_info({}, {}, cmd_buffs, {});
        CHECK_VK_RESULT_R(ctx.queue.submit({submit_info}, {ctx.wait_fence}), false, "Failed to submit to queue");

        CHECK_VK_RESULT_R(ctx.device.waitForFences({ctx.wait_fence}, VK_TRUE, ctx.timeout), false, "Failed to wait for fence");
        ctx.device.resetFences({ctx.wait_fence});

        // Reseting the command pool will also destroy the cmd_buff buffer
        ctx.device.resetCommandPool(ctx.command_pool);

        return true;
    }

} // namespace vk_util
} // namespace aq