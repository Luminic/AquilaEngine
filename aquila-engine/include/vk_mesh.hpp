#ifndef VK_MESH_HPP
#define VK_MESH_HPP

#include <vk_types.hpp>
#include <glm/glm.hpp>

namespace aq {

    struct VertexInputDescription {
        std::vector<vk::VertexInputBindingDescription> bindings;
        std::vector<vk::VertexInputAttributeDescription> attributes;
        vk::PipelineVertexInputStateCreateFlags flags = {};
    };

    struct Vertex {
        glm::vec4 position;
        glm::vec4 normal;
        glm::vec4 color;

        static VertexInputDescription get_vertex_description();
    };

    struct Mesh {
        std::vector<Vertex> vertices;
        AllocatedBuffer vertex_buffer;
    };

}

#endif