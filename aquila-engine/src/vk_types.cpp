#include "vk_types.hpp"

namespace aq {

    AllocatedBuffer::AllocatedBuffer() : buffer(nullptr), allocation(nullptr), allocator(nullptr) {}

    bool AllocatedBuffer::allocate(vma::Allocator* allocator, size_t allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage) {
        this->allocator = allocator;

        vk::BufferCreateInfo buffer_create_info(
            {},
            allocation_size,
            usage,
            vk::SharingMode::eExclusive
        );

        vma::AllocationCreateInfo alloc_create_info({}, memory_usage);

        auto[cb_result, buff_alloc] = allocator->createBuffer(buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT_R(cb_result, false, "Failed to create/allocate mesh");
        set(buff_alloc);

        return true;
    }

    void AllocatedBuffer::destroy() {
        if (allocator) {
            allocator->destroyBuffer(buffer, allocation);
            
            buffer = nullptr;
            allocation = nullptr;

            allocator = nullptr;
        }
    }

    void AllocatedBuffer::set(const std::pair<vk::Buffer, vma::Allocation>& lhs) {
        buffer = lhs.first;
        allocation = lhs.second;
    }

}