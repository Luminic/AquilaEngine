#ifndef SCENE_AQUILA_MESH_HPP
#define SCENE_AQUILA_MESH_HPP

#include <vector>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "scene/aq_vertex.hpp"

namespace aq {

    class Mesh {
    public:
        Mesh();
        Mesh(const std::vector<Vertex>& vertices);
        ~Mesh();

        std::vector<Vertex> vertices;
        std::vector<Index> indices;

        // Stores both the index data and the vertex data in the same buffer
        // The index data is first and the vertex data starts at `vertex_data_offset`
        AllocatedBuffer combined_iv_buffer;
        vk::DeviceSize vertex_data_offset;

        void upload(vma::Allocator* allocator, bool override_prev=false);
        // Will only free `vertex_buffer` if it was allocated through `upload`
        void free(); 

    private:
        vma::Allocator* allocator = nullptr;
    };

}

#endif