#ifndef AQUILA_MESH_HPP
#define AQUILA_MESH_HPP

#include <vk_types.hpp>
#include <aq_vertex.hpp>
#include <glm/glm.hpp>

namespace aq {

    class Mesh {
    public:
        Mesh();
        Mesh(const std::vector<Vertex>& vertices);
        ~Mesh();

        std::vector<Vertex> vertices;
        AllocatedBuffer vertex_buffer;
        std::vector<Index> indices;
        AllocatedBuffer index_buffer;

        void upload(vma::Allocator* allocator, bool override_prev=false);
        // Will only free `vertex_buffer` if it was allocated through `upload`
        void free(); 

    private:
        vma::Allocator* allocator = nullptr;
    };

}

#endif