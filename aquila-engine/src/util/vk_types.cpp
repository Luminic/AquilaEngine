#include "util/vk_types.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "util/vk_utility.hpp"

namespace aq {

    void DeletionQueue::push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void DeletionQueue::flush() {
        for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
            (*it)(); // Call functors
        }
        deletors.clear();
    }

    void DeletionQueue::swap(DeletionQueue other) {
        deletors.swap(other.deletors);
    }


    AllocatedBuffer::AllocatedBuffer() : buffer(nullptr), allocation(nullptr), allocator(nullptr) {}

    bool AllocatedBuffer::allocate(vma::Allocator* allocator, vk::DeviceSize allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage, vk::MemoryPropertyFlags memory_property_flags) {
        vk::BufferCreateInfo buffer_create_info = vk::BufferCreateInfo()
            .setSize(allocation_size)
            .setUsage(usage)
            .setSharingMode(vk::SharingMode::eExclusive);

        vma::AllocationCreateInfo alloc_create_info({}, memory_usage, memory_property_flags);

        auto[cb_result, buff_alloc] = allocator->createBuffer(buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT_R(cb_result, false, "Failed to create/allocate mesh");
        set(buff_alloc);

        this->allocator = allocator;

        return true;
    }

    bool AllocatedBuffer::upload(void* data, vma::Allocator* allocator, vk::DeviceSize allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage, vk::MemoryPropertyFlags memory_property_flags) {
        if (!allocate(allocator, allocation_size, usage, memory_usage, memory_property_flags)) return false;

        auto[mm_result, b_memory] = allocator->mapMemory(allocation);
        CHECK_VK_RESULT_R(mm_result, false, "Failed to map mesh buffer memory");
        memcpy(b_memory, data, allocation_size);
        allocator->flushAllocation(allocation, 0, VK_WHOLE_SIZE);
        allocator->unmapMemory(allocation);

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

    void AllocatedBuffer::set(const std::pair<vk::Buffer, vma::Allocation>& rhs) {
        buffer = rhs.first;
        allocation = rhs.second;
    }

    AllocatedBuffer& AllocatedBuffer::operator=(const AllocatedBuffer& rhs) {
        buffer = rhs.buffer;
        allocation = rhs.allocation;
        allocator = rhs.allocator;
        return *this;
    }


    AllocatedImage::AllocatedImage() {}

    bool AllocatedImage::upload(void* texture_data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        vk::DeviceSize image_size = width * height * 4;
        vk::Format image_format = vk::Format::eR8G8B8A8Unorm;

        // Create the staging buffer

        AllocatedBuffer staging_buffer;
        if (!staging_buffer.upload(texture_data, allocator, image_size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly)) return false;

        // Create the image buffer

        vk::Extent3D extent(width, height, 1);

        vk::ImageCreateInfo image_create_info = vk::ImageCreateInfo()
            .setImageType(vk::ImageType::e2D)
            .setFormat(image_format)
            .setExtent(extent)
            .setMipLevels(1)
            .setArrayLayers(1)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);

        vma::AllocationCreateInfo alloc_create_info({}, vma::MemoryUsage::eGpuOnly);

        auto[ci_res, image_alloc] = allocator->createImage(image_create_info, alloc_create_info);
        set(image_alloc);

        // Transfer contents of staging buffer into image buffer

        vk_util::immediate_submit([this, &staging_buffer, &extent](vk::CommandBuffer cmd) {
            // Transition image to be usable as transfer dst

            vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            vk::ImageMemoryBarrier image_transfer_memory_barrier = vk::ImageMemoryBarrier()
                .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setOldLayout(vk::ImageLayout::eUndefined)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setImage(image)
                .setSubresourceRange(subresource_range);

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eTransfer,
                {}, // dependency flags
                {}, {}, // memory and buffer-memory barriers
                {image_transfer_memory_barrier}
            );

            // Actual data transfer

            vk::BufferImageCopy copy = vk::BufferImageCopy()
                .setImageSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 })
                .setImageExtent(extent);

            cmd.copyBufferToImage(staging_buffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, {copy});

            // Transition image to be readable by shader

            vk::ImageMemoryBarrier image_read_memory_barrier = vk::ImageMemoryBarrier()
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImage(image)
                .setSubresourceRange(subresource_range);

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                {}, // dependency flags
                {}, {}, // memory and buffer-memory barriers
                {image_read_memory_barrier}
            );

        }, upload_context);

        staging_buffer.destroy();

        this->allocator = allocator;

        return true;
    }

    void AllocatedImage::destroy() {
        if (allocator) {
            allocator->destroyImage(image, allocation);
            
            image = nullptr;
            allocation = nullptr;

            allocator = nullptr;
        }
    }

    void AllocatedImage::set(const std::pair<vk::Image, vma::Allocation>& rhs) {
        image = rhs.first;
        allocation = rhs.second;
    }

}