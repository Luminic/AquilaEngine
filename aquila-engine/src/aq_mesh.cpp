#include "aq_mesh.hpp"

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

        vk::BufferCreateInfo vertex_buffer_create_info(
            {},
            vertices.size() * sizeof(Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::SharingMode::eExclusive
        );

        vk::BufferCreateInfo index_buffer_create_info(
            {},
            indices.size() * sizeof(Index),
            vk::BufferUsageFlagBits::eIndexBuffer,
            vk::SharingMode::eExclusive
        );

        vma::AllocationCreateInfo alloc_create_info({}, vma::MemoryUsage::eCpuToGpu);

        auto[cvb_result, vbuff_alloc] = allocator->createBuffer(vertex_buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT(cvb_result, "Failed to create/allocate mesh vertex-buffer");
        vertex_buffer = vbuff_alloc;

        auto[cib_result, ibuff_alloc] = allocator->createBuffer(index_buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT(cib_result, "Failed to create/allocate mesh index-buffer");
        index_buffer = ibuff_alloc;

        // Copy vertex data into buffers

        auto[mvm_result, v_memory] = allocator->mapMemory(vertex_buffer.allocation);
        CHECK_VK_RESULT(mvm_result, "Failed to map vertex-buffer memory");
        memcpy(v_memory, vertices.data(), vertices.size() * sizeof(Vertex));
        allocator->unmapMemory(vertex_buffer.allocation);

        auto[mim_result, i_memory] = allocator->mapMemory(index_buffer.allocation);
        CHECK_VK_RESULT(mim_result, "Failed to map index-buffer memory");
        memcpy(i_memory, indices.data(), indices.size() * sizeof(Index));
        allocator->unmapMemory(index_buffer.allocation);
    }

    void Mesh::free() {
        if (allocator) {
            allocator->destroyBuffer(vertex_buffer.buffer, vertex_buffer.allocation);
            vertex_buffer.buffer = nullptr;
            vertex_buffer.allocation = nullptr;

            allocator->destroyBuffer(index_buffer.buffer, index_buffer.allocation);
            index_buffer.buffer = nullptr;
            index_buffer.allocation = nullptr;
            
            allocator = nullptr;
        }
    }

}