#include "util/vk_types.hpp"

namespace aq {

    class ResizableBuffer {
    public:
        ResizableBuffer();

        // Will add `vk::BufferUsageFlagBits::eTransferSrc` and `vk::BufferUsageFlagBits::eTransferDst` to `usage` 
        bool allocate(vma::Allocator* allocator, vk::DeviceSize allocation_size, const vk::BufferUsageFlags& usage, const vma::MemoryUsage& memory_usage);
        // If `allocator` is nullptr (default) the original allocator will be used
        // Ideally shouldn't be called very often.
        // Resizing where `new_size == old_size` will still recreate the buffer (to use the new allocator if provided).
        bool resize(vk::DeviceSize new_size, const vk_util::UploadContext& ctx, vma::Allocator* allocator=nullptr);

        void destroy(); // Only works if the object was allocated/uploaded with a member function

        vk::Buffer get_buffer() {return buffer.buffer;};
        vma::Allocation get_allocation() {return buffer.allocation;};
        vk::DeviceSize get_size() {return buffer_size;}

    private:
        AllocatedBuffer buffer;
        vk::DeviceSize buffer_size;

        vk::BufferUsageFlags usage_flags;
        vma::MemoryUsage memory_usage_flags;
        vma::Allocator* allocator = nullptr;
    };

}