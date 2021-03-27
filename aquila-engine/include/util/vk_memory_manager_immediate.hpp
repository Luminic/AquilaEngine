#ifndef UTIL_AQUILA_MEMORY_MANAGER_IMMEDIATE_HPP
#define UTIL_AQUILA_MEMORY_MANAGER_IMMEDIATE_HPP

#include <vector>
#include <memory>
#include <utility>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "util/vk_resizable_buffer.hpp"

namespace aq {

    class MemoryManagerImmediate {
    public:
        MemoryManagerImmediate();

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
        // `object` will only be uploaded into the buffer when `update()` is called
        // Returns the index `object` will be located at.
        ManagedMemoryIndex add_object(const void* object);

        // Uploads object memory added through `add_object` to the GPU for `safe_frame`
        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        // Returns the number of objects uploaded to the GPU
        size_t update(uint safe_frame);

        // Ensures the buffer for `safe_frame` has enough space for `nr_objects` objects
        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        void reserve(size_t nr_objects, uint safe_frame);

        // `object` should be a pointer to `object_size * nr_objects` bytes of memory.
        // Uploads object directly into the buffer at `index` for `safe_frame`.
        // This function DOES NOT ensure the buffer is large enough to put `object` at `index`. You must do that yourself using `reserve()`
        // This function DOES NOT communicate with `add_object`. Use only `add_object()`+`update()` or  `add_object_direct()`. Do not mix them.
        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        void add_object_direct(size_t index, const void* object, uint safe_frame, size_t nr_objects=1);
    
    private:
        struct Buffer {
            ResizableBuffer buffer;
            std::byte* buff_mem;
            vk::WriteDescriptorSet write_descriptor;
        };
        std::vector<Buffer> buffers;
        std::vector<std::byte> update_queue;

        uint object_size;
        size_t nr_objects;
        size_t buffer_size; // Might not be accurate; buffer sizes are updated in `update`

        vma::Allocator* allocator;
        vk_util::UploadContext ctx;
    };

}

#endif