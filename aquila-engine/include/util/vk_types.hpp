#ifndef UTIL_AQUILA_VK_TYPES_HPP
#define UTIL_AQUILA_VK_TYPES_HPP

#include <iostream>
#include <deque>

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

    class DeletionQueue {
    public:
        void push_function(std::function<void()>&& function);
        void flush();
        void swap(DeletionQueue other);
        
    private:
        std::deque<std::function<void()>> deletors;
    };

    class AllocatedBuffer {
    public:
        AllocatedBuffer();

        bool allocate(vma::Allocator* allocator, vk::DeviceSize allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage);
        // Buffer memory must be CPU writeable
        bool upload(void* data, vma::Allocator* allocator, vk::DeviceSize allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage);
        void destroy(); // Only works if the object was allocated/uploaded with a member function

        void set(const std::pair<vk::Buffer, vma::Allocation>& rhs);
        AllocatedBuffer& operator=(const AllocatedBuffer& rhs);

        vk::Buffer buffer;
        vma::Allocation allocation;
    
    private:
        vma::Allocator* allocator = nullptr;
    };

    struct AllocatedImage {
        AllocatedImage();

        bool upload(void* data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        void destroy(); // Only works if the object was allocated/uploaded with a member function

        void set(const std::pair<vk::Image, vma::Allocation>& rhs);

        vk::Image image;
        vma::Allocation allocation;

    private:
        vma::Allocator* allocator = nullptr;
    };

}

#endif
