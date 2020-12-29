#ifndef AQUILA_VERTEX_HPP
#define AQUILA_VERTEX_HPP

#include "vk_types.hpp"

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

    using Index = uint32_t;
    constexpr vk::IndexType index_vk_type = vk::IndexType::eUint32;

}

#endif