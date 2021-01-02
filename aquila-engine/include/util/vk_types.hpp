#ifndef UTIL_AQUILA_VK_TYPES_HPP
#define UTIL_AQUILA_VK_TYPES_HPP

#include <iostream>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT
#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.hpp>

#define CHECK_VK_RESULT(vk_result, msg) do { \
        if ((vk_result) != vk::Result::eSuccess) { \
            std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
        } \
    } while (0)

#define CHECK_VK_RESULT_R(vk_result, return_if_err, msg) do { \
        if ((vk_result) != vk::Result::eSuccess) { \
            std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
            return (return_if_err); \
        } \
    } while (0)

namespace aq {

    namespace vk_util {
        struct UploadContext {
            vk::Fence wait_fence;
            uint64_t timeout;
            vk::CommandPool command_pool;
            vk::Queue queue;
            vk::Device device;
        };
    }

    class AllocatedBuffer {
    public:
        AllocatedBuffer();

        bool allocate(vma::Allocator* allocator, vk::DeviceSize allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage);
        void destroy(); // Only works if the object was allocated with `allocate`

        void set(const std::pair<vk::Buffer, vma::Allocation>& lhs);

        vk::Buffer buffer;
        vma::Allocation allocation;
    
    private:
        vma::Allocator* allocator;
    };

    struct AllocatedImage {
        AllocatedImage();

        bool upload_from_data(void* data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        void destroy(); // Only works if the object was allocated/uploaded with a member function

        void set(const std::pair<vk::Image, vma::Allocation>& lhs);

        vk::Image image;
        vma::Allocation allocation;

    private:
        vma::Allocator* allocator;
    };

}

#endif
