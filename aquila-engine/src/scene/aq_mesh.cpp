#include "scene/aq_mesh.hpp"

#include "util/vk_utility.hpp"

namespace aq {

    Mesh::Mesh() {}

    Mesh::Mesh(const std::vector<Vertex>& vertices) : vertices(vertices) {}

    Mesh::~Mesh() {
        free(); // Just in case
    }

    void Mesh::upload(vma::Allocator* allocator) {
        if (this->allocator) {
            std::cerr << "Request to upload already uploaded mesh. Request ignored.";
            return;
        }
        this->allocator = allocator;

        combined_iv_buffer = create_buffer_with_iv_data(
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
            vma::MemoryUsage::eCpuToGpu
        );
    }

    void Mesh::upload(vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        if (this->allocator) {
            std::cerr << "Request to upload already uploaded mesh. Request ignored.";
            return;
        }
        this->allocator = allocator;

        vertex_data_offset = indices.size() * sizeof(Index);
        vk::DeviceSize buffer_size = indices.size()*sizeof(Index) + vertices.size()*sizeof(Vertex);

        AllocatedBuffer staging_buffer = create_buffer_with_iv_data(
            vk::BufferUsageFlagBits::eTransferSrc,
            vma::MemoryUsage::eCpuOnly
        );

        // Create actual buffer

        combined_iv_buffer.allocate(allocator, buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vma::MemoryUsage::eGpuOnly);

        // Transfer contents of staging buffer into actual buffer

        vk_util::immediate_submit([this, buffer_size, &staging_buffer](vk::CommandBuffer cmd) {
            vk::BufferCopy buffer_copy(0, 0, buffer_size);
            cmd.copyBuffer(staging_buffer.buffer, combined_iv_buffer.buffer, {buffer_copy});
        }, upload_context);

        staging_buffer.destroy();
    }

    AllocatedBuffer Mesh::create_buffer_with_iv_data(vk::BufferUsageFlags buffer_usage, vma::MemoryUsage memory_usage) {
        vertex_data_offset = indices.size() * sizeof(Index);
        vk::DeviceSize iv_buffer_size = indices.size()*sizeof(Index) + vertices.size()*sizeof(Vertex);

        AllocatedBuffer buffer_with_data{};
        buffer_with_data.allocate(allocator, iv_buffer_size, buffer_usage, memory_usage, vk::MemoryPropertyFlagBits::eHostCoherent);
        
        // Copy data into buffer

        auto[mm_result, b_memory] = allocator->mapMemory(buffer_with_data.allocation);
        CHECK_VK_RESULT(mm_result, "Failed to map mesh buffer memory");
        memcpy(b_memory, indices.data(), indices.size() * sizeof(Index));
        memcpy((char*)b_memory + vertex_data_offset, vertices.data(), vertices.size() * sizeof(Vertex));
        allocator->unmapMemory(buffer_with_data.allocation);

        return buffer_with_data;
    }

    void Mesh::free() {
        combined_iv_buffer.destroy();
        vertex_data_offset = 0;
        allocator = nullptr;
        material = {};
    }

}