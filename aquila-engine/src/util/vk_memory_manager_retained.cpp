#include "util/vk_memory_manager_retained.hpp"

#include <iostream>

namespace aq {

    MemoryManagerRetained::MemoryManagerRetained() {}

    bool MemoryManagerRetained::init(
        uint frame_overlap,
        size_t object_size,
        size_t reserve_size,
        vk::WriteDescriptorSet* descriptors,
        vma::Allocator* allocator, 
        vk_util::UploadContext upload_context
    ) {
        this->object_size = object_size;
        buffer_size = reserve_size;
        end_index = 0;

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

    void MemoryManagerRetained::destroy() {
        for (auto& buffer : buffers) {
            buffer.buffer.destroy();
        }
        buffers.clear();
        free_memory.clear();
    }

    ManagedMemoryIndex MemoryManagerRetained::add_object(void* object) {
        ManagedMemoryIndex object_index;
        if (!free_memory.empty()) {
            object_index = free_memory.front();
            free_memory.pop_front();
        } else {
            object_index = end_index;
            end_index++;
        }

        std::shared_ptr<std::byte[]> memory(new std::byte[object_size]);
        memcpy(memory.get(), object, object_size);
        for (auto& buffer : buffers) {
            buffer.update_queue.push_back({Action::Type::Add, 1, object_index, memory});
        }
        return object_index;
    }

    void MemoryManagerRetained::update_object(ManagedMemoryIndex index, void* object) {
        std::shared_ptr<std::byte[]> memory(new std::byte[object_size]);
        memcpy(memory.get(), object, object_size);
        for (auto& buffer : buffers) {
            buffer.update_queue.push_back({Action::Type::Update, 1, index, memory});
        }
    }

    void MemoryManagerRetained::remove_object(ManagedMemoryIndex index) {
        // Make sure `index` isn't already in `free_memory`
        if (std::find(free_memory.begin(), free_memory.end(), index) == free_memory.end()) {
            free_memory.push_back(index);
            // Currently I do nothing with the memory of removed objects
            // for (auto& buffer : buffers) {    
            //     buffer.update_queue.push_back({Action::Type::Remove, 1, index, nullptr});
            // }
        }
    }

    void MemoryManagerRetained::update(uint safe_frame) {
        auto& buffer = buffers[safe_frame];
        for (auto& action : buffer.update_queue) {
            switch (action.type) {
            case Action::Type::Add: {
                // Make sure the buffer has enough space
                size_t size_needed = action.index + action.count;
                size_t bytes_needed = size_needed * object_size;
                if (bytes_needed > buffer.buffer.get_size()) { // We need to resize
                    if (size_needed > buffer_size) { // Calculate new `buffer_size`
                        buffer_size = size_needed*3/2;
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
            } // Intentional fallthrough to `Action::Type::Update`
            case Action::Type::Update: {
                assert((action.index + action.count) * object_size <= buffer.buffer.get_size()); // Make sure the location is valid
                size_t memory_offset = action.index * object_size;
                memcpy(buffer.buff_mem+memory_offset, action.memory.get(), action.count*object_size);
            } break;
            case Action::Type::Remove:
                // Do nothing. If we really want to be safe, we can 0 the memory
                break;
            }
        }
        buffer.update_queue.clear();
    }

}