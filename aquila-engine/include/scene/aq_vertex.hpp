#ifndef SCENE_AQUILA_VERTEX_HPP
#define SCENE_AQUILA_VERTEX_HPP

#include <glm/glm.hpp>

#include "util/vk_types.hpp"

namespace aq {

    struct Vertex {
        Vertex(
            glm::vec4 position = glm::vec4(0.0f,0.0f,0.0f,1.0f), 
            glm::vec4 normal = glm::vec4(0.0f), 
            glm::vec2 tex_coord = glm::vec4(0.0f)
        );

        glm::vec4 position;
        glm::vec4 normal;
        glm::vec2 tex_coord;

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