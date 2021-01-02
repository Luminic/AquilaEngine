#include "util/vk_types.hpp"

#include "util/vk_utility.hpp"

namespace aq {

    AllocatedBuffer::AllocatedBuffer() : buffer(nullptr), allocation(nullptr), allocator(nullptr) {}

    bool AllocatedBuffer::allocate(vma::Allocator* allocator, vk::DeviceSize allocation_size, vk::BufferUsageFlags usage, vma::MemoryUsage memory_usage) {
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


    AllocatedImage::AllocatedImage() {}

    bool AllocatedImage::upload_from_data(void* texture_data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        this->allocator = allocator;

        vk::DeviceSize image_size = width * height * 4;
        vk::Format image_format = vk::Format::eR8G8B8A8Unorm;

        // Create the staging buffer

        AllocatedBuffer staging_buffer;
        if (!staging_buffer.allocate(allocator, image_size, vk::BufferUsageFlagBits::eTransferSrc, vma::MemoryUsage::eCpuOnly)) return false;

        // Copy image data into the staging buffer

        auto[mm_res, buffer_data] = allocator->mapMemory(staging_buffer.allocation);
        CHECK_VK_RESULT_R(mm_res, false, "Failed to map image staging buffer memory");

        memcpy(buffer_data, texture_data, image_size);

        allocator->unmapMemory(staging_buffer.allocation);

        // Create the image buffer

        vk::Extent3D extent(width, height, 1);

        vk::ImageCreateInfo image_create_info(
            {}, // flags
            vk::ImageType::e2D,
            image_format,
            extent,
            1, 1, // mip and array layers
            vk::SampleCountFlagBits::e1, 
            vk::ImageTiling::eOptimal, 
            vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst
        );

        vma::AllocationCreateInfo alloc_create_info({}, vma::MemoryUsage::eGpuOnly);

        auto[ci_res, image_alloc] = allocator->createImage(image_create_info, alloc_create_info);
        set(image_alloc);

        // Transfer contents of staging buffer into image buffer

        vk_util::immediate_submit([this, &staging_buffer, &extent](vk::CommandBuffer cmd) {
            // Transition image to be usable as transfer dst

            vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            vk::ImageMemoryBarrier image_transfer_memory_barrier(
                {}, // src access mask
                vk::AccessFlagBits::eTransferWrite, // dst access mask
                vk::ImageLayout::eUndefined, // old layout
                vk::ImageLayout::eTransferDstOptimal, // new layout
                0, 0, // drc & dst queue family indices (no change so set both to 0)
                image,
                subresource_range
            );

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eTransfer,
                {}, // dependency flags
                {}, {}, // memory and buffer-memory barriers
                {image_transfer_memory_barrier}
            );

            // Actual data transfer

            vk::BufferImageCopy copy(
                0, // buffer offset
                0, 0, // row length, and image height
                { vk::ImageAspectFlagBits::eColor, 0, 0, 1 }, // image subresource
                { 0, 0, 0 }, // image offset
                extent
            );

            cmd.copyBufferToImage(staging_buffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, {copy});

            // Transition image to be readable by shader

            vk::ImageMemoryBarrier image_read_memory_barrier(
                vk::AccessFlagBits::eTransferWrite, // src access mask
                vk::AccessFlagBits::eShaderRead, // dst access mask
                vk::ImageLayout::eTransferDstOptimal, // old layout
                vk::ImageLayout::eShaderReadOnlyOptimal, // new layout
                0, 0, // drc & dst queue family indices (no change so set both to 0)
                image,
                subresource_range
            );

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                {}, // dependency flags
                {}, {}, // memory and buffer-memory barriers
                {image_read_memory_barrier}
            );

        }, upload_context);

        staging_buffer.destroy();

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

    void AllocatedImage::set(const std::pair<vk::Image, vma::Allocation>& lhs) {
        image = lhs.first;
        allocation = lhs.second;
    }

}