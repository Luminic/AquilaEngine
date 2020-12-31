#include "scene/aq_vertex.hpp"

namespace aq {

    Vertex::Vertex(glm::vec4 position, glm::vec4 normal, glm::vec4 color)
    : position(position), normal(normal), color(color) {}

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

}