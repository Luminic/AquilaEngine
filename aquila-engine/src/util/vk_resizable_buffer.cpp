#include "util/vk_resizable_buffer.hpp"

#include <algorithm>

#include "util/vk_utility.hpp"

namespace aq {

    ResizableBuffer::ResizableBuffer() {}

    bool ResizableBuffer::allocate(vma::Allocator* allocator, vk::DeviceSize allocation_size, const vk::BufferUsageFlags& usage, const vma::MemoryUsage& memory_usage) {
        usage_flags = usage | vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
        memory_usage_flags = memory_usage;
        this->allocator = allocator;
        buffer_size = allocation_size;

        return buffer.allocate(allocator, allocation_size, usage, memory_usage);
    }

    bool ResizableBuffer::resize(vk::DeviceSize new_size, const vk_util::UploadContext& ctx, vma::Allocator* allocator) {
        if (allocator) this->allocator = allocator;

        // Create the new buffer

        AllocatedBuffer new_buffer{};
        new_buffer.allocate(this->allocator, new_size, usage_flags, memory_usage_flags);

        // Transfer contents of the old buffer into the new one

        vk::DeviceSize copy_size = std::min(buffer_size, new_size);
        vk_util::immediate_submit([this, &copy_size, &new_buffer](vk::CommandBuffer cmd) {
            vk::BufferCopy buffer_copy(0, 0, copy_size);
            cmd.copyBuffer(buffer.buffer, new_buffer.buffer, {buffer_copy});
        }, ctx);

        // Destroy the old buffer and replace with the new buffer

        buffer.destroy();
        buffer = new_buffer;
        buffer_size = new_size;

        return true;
    }

    void ResizableBuffer::destroy() {
        buffer.destroy();
    }

}