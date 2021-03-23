#ifndef UTIL_AQUILA_MEMORY_MANAGER_HPP
#define UTIL_AQUILA_MEMORY_MANAGER_HPP

#include <vector>
#include <list>
#include <deque>
#include <unordered_map>
#include <memory>
#include <utility>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "util/vk_resizable_buffer.hpp"
#include "scene/aq_texture.hpp"

namespace aq {
    
    using ManagedMemoryIndex = size_t;

    class MemoryManager {
    public:
        MemoryManager();

        bool init(
            uint frame_overlap,
            size_t object_size,
            size_t reserve_size, // Number of objects the first allocation should be able to hold
            vk::WriteDescriptorSet* descriptors, // A pointer to `frame_overlap` `vk::WriteDescriptorSet`s. The memory only needs to stay valid until `init` returns. `pBufferInfo` and `descriptorType` will be filled in by `MaterialManager`.
            vma::Allocator* allocator, 
            vk_util::UploadContext upload_context
        );

        // Release allocated data.
        void destroy();

        // `object` should be a pointer to `object_size` bytes of memory.
        // `object` will be copied; it only needs to be a valid pointer until this function returns
        // Returns the index `object` will be located at.
        ManagedMemoryIndex add_object(void* object);

        // `object` should be a pointer to `object_size` bytes of memory
        // `object` will be copied; it only needs to be a valid pointer until this function returns
        // Updates object at `index`.
        void update_object(ManagedMemoryIndex index, void* object);

        // Marks memory at `index` free
        // `index` will no longer be valid.
        void remove_object(ManagedMemoryIndex index);

        // Uploads object memory to the GPU for `safe_frame`
        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        void update(uint safe_frame);

    private:
        struct Action {
            enum class Type {
                Add,
                Update,
                Remove
            };
            Type type;
            uint count = 1;
            size_t index; 

            // `memory` should be an array of size `object_size*count` bytes (uchars)
            std::shared_ptr<std::byte[]> memory;
        };

        struct QueuedBuffer {
            ResizableBuffer buffer;
            std::byte* buff_mem;
            vk::WriteDescriptorSet write_descriptor;

            std::vector<Action> update_queue;
        };
        std::vector<QueuedBuffer> buffers;

        // Holes in the memory caused by `remove_object` that can be filled
        std::deque<ManagedMemoryIndex> free_memory;

        uint object_size;
        size_t buffer_size; // Might not be accurate; buffer sizes are updated in `update`
        size_t end_index; // End of where the buffer is filled. Memory at `end_index` and beyond is free

        vma::Allocator* allocator;
        vk_util::UploadContext ctx;
    };

}

#endif