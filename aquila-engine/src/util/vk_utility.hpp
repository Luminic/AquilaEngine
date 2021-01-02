#ifndef UTIL_AQUILLA_VK_UTILITY
#define UTIL_AQUILLA_VK_UTILITY

#include "util/vk_types.hpp"

namespace aq {
namespace vk_util {

    // Algorithm from Sascha Willems (https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer)
    inline size_t pad_uniform_buffer_size(size_t original_size, vk::DeviceSize min_ubo_alignment) {
        // Calculate required alignment based on minimum device offset alignment
        size_t aligned_size = original_size;
        if (min_ubo_alignment > 0) {
            aligned_size = (aligned_size + min_ubo_alignment - 1) & ~(min_ubo_alignment - 1);
        }
        return aligned_size;
    }

    bool immediate_submit(std::function<void(vk::CommandBuffer)>&& function, const UploadContext& ctx);

} // namespace vk_util
} // namespace aq

#endif