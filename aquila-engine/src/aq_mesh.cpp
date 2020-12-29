#include "aq_mesh.hpp"

namespace aq {

    Vertex::InputDescription Vertex::get_vertex_description() {
        vk::VertexInputBindingDescription main_binding(0, sizeof(Vertex), vk::VertexInputRate::eVertex);

        vk::VertexInputAttributeDescription position_attribute(
            0, // location
            0, // binding
            vk::Format::eR32G32B32A32Sfloat,
            offsetof(Vertex, position)
        );

        vk::VertexInputAttributeDescription normal_attribute(
            1, // location
            0, // binding
            vk::Format::eR32G32B32A32Sfloat,
            offsetof(Vertex, normal)
        );

        vk::VertexInputAttributeDescription color_attribute(
            2, // location
            0, // binding
            vk::Format::eR32G32B32A32Sfloat,
            offsetof(Vertex, color)
        );

        return InputDescription{{main_binding}, {position_attribute, normal_attribute, color_attribute}};
    }

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

        vk::BufferCreateInfo buffer_create_info(
            {},
            vertices.size() * sizeof(Vertex),
            vk::BufferUsageFlagBits::eVertexBuffer,
            vk::SharingMode::eExclusive
        );

        vma::AllocationCreateInfo alloc_create_info({}, vma::MemoryUsage::eCpuToGpu);

        auto[cb_result, buff_alloc] = allocator->createBuffer(buffer_create_info, alloc_create_info);
        CHECK_VK_RESULT(cb_result, "Failed to create/allocate mesh vertex-buffer");
        vertex_buffer = buff_alloc;

        // Copy vertex data into vertex buffer

        auto[mm_result, memory] = allocator->mapMemory(vertex_buffer.allocation);
        memcpy(memory, vertices.data(), vertices.size() * sizeof(Vertex));
        allocator->unmapMemory(vertex_buffer.allocation);
    }

    void Mesh::free() {
        if (allocator) {
            allocator->destroyBuffer(vertex_buffer.buffer, vertex_buffer.allocation);
            vertex_buffer.buffer = nullptr;
            vertex_buffer.allocation = nullptr;
            allocator = nullptr;
        }
    }

}