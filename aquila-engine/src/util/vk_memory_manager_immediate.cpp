#include "util/vk_memory_manager_immediate.hpp"

namespace aq {

    MemoryManagerImmediate::MemoryManagerImmediate() {}

    bool MemoryManagerImmediate::init(
        uint frame_overlap,
        size_t object_size,
        size_t reserve_size,
        vk::WriteDescriptorSet* descriptors,
        vma::Allocator* allocator, 
        vk_util::UploadContext upload_context
    ) {
        this->object_size = object_size;
        buffer_size = reserve_size;
        nr_objects = 0;

        this->allocator = allocator;
        ctx = upload_context;

        buffers.resize(frame_overlap);
        vk::DeviceSize allocation_size = buffer_size * object_size;
        for (auto i=0; i<frame_overlap; ++i) {
            auto& buffer = buffers[i];

            buffer.buffer.allocate(allocator, allocation_size, vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eCpuToGpu, vk::MemoryPropertyFlagBits::eHostCoherent);

            auto mm_res = allocator->mapMemory(buffer.buffer.get_allocation(), (void**) &buffer.buff_mem);
            CHECK_VK_RESULT_R(mm_res, false, "Failed to map buffer");

            vk::DescriptorBufferInfo buffer_info(buffer.buffer.get_buffer(), 0, allocation_size);
            buffer.write_descriptor = *(descriptors + i);
            buffer.write_descriptor
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setDescriptorCount(1)
                .setPBufferInfo(&buffer_info);
            ctx.device.updateDescriptorSets({buffer.write_descriptor}, {});
        }

        return true;
    }

    void MemoryManagerImmediate::destroy() {
        for (auto& buffer : buffers) {
            buffer.buffer.destroy();
        }
        buffers.clear();
        update_queue.clear();
    }

    ManagedMemoryIndex MemoryManagerImmediate::add_object(const void* object) {
        update_queue.insert(update_queue.end(), (std::byte*)object, (std::byte*)object+object_size);
        return nr_objects++;
    }

    void MemoryManagerImmediate::update(uint safe_frame) {
        auto& buffer = buffers[safe_frame];

        // Make sure the buffer has enough space
        reserve(nr_objects, safe_frame);

        // Actually update the buffer
        assert(update_queue.size() == nr_objects * object_size);
        memcpy(buffer.buff_mem, update_queue.data(), update_queue.size());
        update_queue.clear();
        nr_objects = 0;
    }

    void MemoryManagerImmediate::reserve(size_t nr_objects, uint safe_frame) {
        auto& buffer = buffers[safe_frame];

        size_t bytes_needed = nr_objects * object_size;
        if (bytes_needed > buffer.buffer.get_size()) { // We need to resize
            if (nr_objects > buffer_size) { // Calculate new `buffer_size`
                buffer_size = nr_objects*3/2;
            }
            // Resize buffer to have enough space
            vk::DeviceSize new_byte_size = buffer_size * object_size;
            if (!buffer.buffer.resize(new_byte_size, ctx)) std::cerr << "Failed to resize resizeable buffer." << std::endl;
            // Get the pointer to the new memory
            auto mm_res = allocator->mapMemory(buffer.buffer.get_allocation(), (void**) &buffer.buff_mem);
            CHECK_VK_RESULT(mm_res, "Failed to map buffer memory after resize.");
            // Make sure to update descriptor sets too
            vk::DescriptorBufferInfo buffer_info(buffer.buffer.get_buffer(), 0, new_byte_size);
            buffer.write_descriptor.setPBufferInfo(&buffer_info);
            ctx.device.updateDescriptorSets({buffer.write_descriptor}, {});
        }
    }

    void MemoryManagerImmediate::add_object_direct(size_t index, const void* object, uint safe_frame, size_t nr_objects) {
        memcpy(buffers[safe_frame].buff_mem + index*object_size, object, object_size*nr_objects);
    }

}