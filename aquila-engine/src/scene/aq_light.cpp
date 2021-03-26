#include "scene/aq_light.hpp"

namespace aq {

    Light::Light(
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        glm::mat4 org_transform
    ) : Node(position, rotation, scale, org_transform) {}

    Light::Light(
        const std::string& name,
        glm::vec3 position,
        glm::quat rotation,
        glm::vec3 scale,
        glm::mat4 org_transform
    ) : Node(name, position, rotation, scale, org_transform) {}

    PointLight::PointLight(
        glm::vec3 position,
        glm::vec4 color
    ) : color(color), Light(position) {}

    PointLight::PointLight(
        const std::string& name,
        glm::vec3 position,
        glm::vec4 color
    ) : color(color), Light(name, position) {}

    PointLight::~PointLight() {}

    Light::Properties PointLight::get_properties(glm::mat4 parent_transform) {
        glm::mat4 transform = parent_transform * get_model_matrix();
        glm::vec3 position = transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        return {
            color,
            position,
            get_type(),
            glm::vec3(0.0f), // direction
            0,  // shadow_map_ti
        };
    }

}