#ifndef VK_MESH_HPP
#define VK_MESH_HPP

#include <vk_types.hpp>
#include <glm/glm.hpp>

namespace aq {

    struct Vertex {
        glm::vec4 position;
        glm::vec4 normal;
        glm::vec4 color;

        struct InputDescription;
        static InputDescription get_vertex_description();
    };

    struct Vertex::InputDescription {
        std::vector<vk::VertexInputBindingDescription> bindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;
        vk::PipelineVertexInputStateCreateFlags flags = {};
    };

    class Mesh {
    public:
        Mesh();
        Mesh(const std::vector<Vertex>& vertices);
        ~Mesh();

        std::vector<Vertex> vertices;
        AllocatedBuffer vertex_buffer;

        void upload(vma::Allocator* allocator, bool override_prev=false);
        // Will only free `vertex_buffer` if it was allocated through `upload`
        void free(); 

    private:
        vma::Allocator* allocator = nullptr;
    };

}

#endif