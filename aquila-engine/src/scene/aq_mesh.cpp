#include "scene/aq_mesh.hpp"

namespace aq {

    Mesh::Mesh() {}

    Mesh::Mesh(const std::vector<Vertex>& vertices) : vertices(vertices) {}

    Mesh::~Mesh() {
        free(); // Just in case
    }

    void Mesh::upload(vma::Allocator* allocator, bool override_prev) {
        if (this->allocator) {
            if (override_prev) free();
            else return;
        }
        this->allocator = allocator;

        vertex_data_offset = indices.size() * sizeof(Index);

        vk::BufferCreateInfo combined_buffer_create_info(
            {},
            indices.size()*sizeof(Index) + vertices.size()*sizeof(Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
            vk::SharingMode::eExclusive
        );

        vma::AllocationCreateInfo alloc_create_info({}, vma::MemoryUsage::eCpuToGpu);

        auto[cb_result, ivbuff_alloc] = allocator->createBuffer(combined_buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT(cb_result, "Failed to create/allocate mesh combined vertex-index buffer");
        combined_iv_buffer.set(ivbuff_alloc);

        // Copy data into buffer
        auto[mm_result, b_memory] = allocator->mapMemory(combined_iv_buffer.allocation);
        CHECK_VK_RESULT(mm_result, "Failed to map combined vertex-index buffer memory");
        memcpy(b_memory, indices.data(), indices.size() * sizeof(Index));
        memcpy((char*)b_memory + vertex_data_offset, vertices.data(), vertices.size() * sizeof(Vertex));
        allocator->unmapMemory(combined_iv_buffer.allocation);

    }

    void Mesh::free() {
        if (allocator) {
            allocator->destroyBuffer(combined_iv_buffer.buffer, combined_iv_buffer.allocation);
            combined_iv_buffer.buffer = nullptr;
            combined_iv_buffer.allocation = nullptr;
            
            allocator = nullptr;
        }
    }

}