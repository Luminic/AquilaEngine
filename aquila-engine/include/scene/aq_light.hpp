#ifndef SCENE_AQUILA_LIGHT_HPP
#define SCENE_AQUILA_LIGHT_HPP

#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

#include <glm/glm.hpp>

#include "util/vk_memory_manager_immediate.hpp"
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
        virtual Properties get_properties() { return get_properties(parent_transform_cache); }

        virtual void hierarchical_update(uint64_t frame_number, const glm::mat4& parent_transform) override;

    protected:
        glm::mat4 parent_transform_cache;
        uint64_t last_hierarchical_update; // Used to warn user if there are multiple instances of `PointLight` in hierarchy
    };

    class LightManager {
    public:
        LightManager();

        // Initialize the `LightManager`
        // Call `descriptor_sets_created(...)` after `per_frame_descriptor_set_builder.build()`
        void init(
            uint frame_overlap,
            size_t initial_capacity,
            DescriptorSetBuilder& per_frame_descriptor_set_builder, // should have multiplicity of `frame_overlap`
            vma::Allocator* allocator,
            vk_util::UploadContext upload_context
        );

        // Call after `per_frame_descriptor_set_builder.build()` where `per_frame_descriptor_set_builder` is the same one from `init`
        // Will update the descriptor sets
        // `descriptor_sets` should have size `frame_overlap` 
        void descriptor_sets_created(const std::vector<vk::DescriptorSet>& descriptor_sets);

        void destroy();

        // Returns false if `light` is not being managed (has not been added)
        bool add_light(std::shared_ptr<Light> light); // Light memory will not actually be uploaded to the GPU until `update` is called
        bool remove_light(std::shared_ptr<Light> light);

        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        // Uploads light memory to the buffer for `safe_frame`
        void update(uint safe_frame);

        size_t get_nr_lights() const { return lights.size(); }

    private:
        MemoryManagerImmediate light_memory;
        std::unordered_set<std::shared_ptr<Light>> lights;

        uint frame_overlap;
        size_t initial_capacity;

        std::vector<vk::DescriptorSet> descriptor_sets;

        vma::Allocator* allocator;
        vk_util::UploadContext ctx;
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