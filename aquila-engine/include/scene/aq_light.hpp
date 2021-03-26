#ifndef SCENE_AQUILA_LIGHT_HPP
#define SCENE_AQUILA_LIGHT_HPP

#include "scene/aq_node.hpp"

namespace aq {

    class Light : public Node {
    public:
        Light(
            glm::vec3 position=glm::vec3(0.0f), 
            glm::quat rotation=glm::quat(1.0f,0.0f,0.0f,0.0f), 
            glm::vec3 scale=glm::vec3(1.0f),
            glm::mat4 org_transform=glm::mat4(1.0f)
        );
        Light(
            const std::string& name,
            glm::vec3 position=glm::vec3(0.0f), 
            glm::quat rotation=glm::quat(1.0f,0.0f,0.0f,0.0f), 
            glm::vec3 scale=glm::vec3(1.0f),
            glm::mat4 org_transform=glm::mat4(1.0f)
        );
        virtual ~Light() = default;

        enum class Type : uint {
            Point = 0,
            Sun   = 1, // Currently Unimplemented
            Area  = 2, // Currently Unimplemented
            Spot  = 3, // Currently Unimplemented
            Other = UINT_MAX
        };
        virtual Type get_type() const = 0;

        // Every light *should* be able to be represented by Light::Properties
        // Order of variables is messy to pack everything tightly
        struct Properties {
            glm::vec4 color = glm::vec4(1.0f); // r,g,b,strength

            glm::vec3 position;
            Type type;

            glm::vec3 direction;
            uint shadow_map_ti;

            // Misc data to be used for different purposes depending on the light type
            glm::vec4 misc;
        };
        Properties properties;

        virtual Properties get_properties(glm::mat4 parent_transform) = 0;
    };

    class LightManager {
    public:
        LightManager();

        bool init(
            uint frame_overlap, 
            vma::Allocator* allocator, 
            vk_util::UploadContext upload_context,
            size_t initial_capacity=10
        );
    };

    class PointLight : public Light {
    public:
        PointLight(
            glm::vec3 position=glm::vec3(0.0f),
            glm::vec4 color=glm::vec4(1.0f)
        );
        PointLight(
            const std::string& name,
            glm::vec3 position=glm::vec3(0.0f),
            glm::vec4 color=glm::vec4(1.0f)
        );
        virtual ~PointLight();

        virtual Type get_type() const override { return Light::Type::Point; };
        virtual Properties get_properties(glm::mat4 parent_transform) override;

        glm::vec4 color;
    };

}

#endif