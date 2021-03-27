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

    class LightMemoryManager;

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

        virtual Properties get_properties(glm::mat4 parent_transform) = 0;

        virtual void hierarchical_update(uint64_t frame_number, const glm::mat4& parent_transform) override;

        virtual Light& set_memory_manager(LightMemoryManager* memory_manager) { this->memory_manager=memory_manager; return *this; }
        virtual LightMemoryManager* get_memory_manager() { return memory_manager; }

    protected:
        LightMemoryManager* memory_manager = nullptr;
        uint64_t last_hierarchical_update = UINT64_MAX; // Used to warn user if there are multiple instances of `PointLight` in hierarchy
    };

    // A bit different from `MaterialManager` in the way that:
    // `MaterialManager` gets the pointer to `Material` while
    // `Light` gets the pointer to `LightMemoryManager`
    class LightMemoryManager {
    public:
        LightMemoryManager();

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

        void add_light(const Light::Properties& light_properties); // Light memory will not actually be uploaded to the GPU until `update` is called

        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        // Uploads light memory to the buffer for `safe_frame`
        // Returns the number of lights uploaded
        size_t update(uint safe_frame);

    private:
        MemoryManagerImmediate light_memory;

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