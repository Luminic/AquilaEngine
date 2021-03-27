#include "scene/aq_light.hpp"

#include <iostream>

#include "util/vk_shaders.hpp"

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

    void Light::hierarchical_update(uint64_t frame_number, const glm::mat4& parent_transform) {
        if (last_hierarchical_update == frame_number) std::cerr << "There can only be one Light instance in a hierarchy.\n";
        parent_transform_cache = parent_transform;
        last_hierarchical_update = frame_number;
    }


    LightManager::LightManager() {}

    void LightManager::init(
        uint frame_overlap,
        size_t initial_capacity,
        DescriptorSetBuilder& per_frame_descriptor_set_builder, // should have multiplicity of `frame_overlap`
        vma::Allocator* allocator,
        vk_util::UploadContext upload_context
    ) {
        this->frame_overlap = frame_overlap;
        this->initial_capacity = initial_capacity;
        this->allocator = allocator;
        ctx = upload_context;

        per_frame_descriptor_set_builder.add_binding({(int) PerFrameBufferBindings::LightPropertiesBuffer, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment});
    }

    void LightManager::descriptor_sets_created(const std::vector<vk::DescriptorSet>& descriptor_sets) {
        this->descriptor_sets = descriptor_sets;

        // Init material buffers

        std::vector<vk::WriteDescriptorSet> write_buff_desc_sets;
        for (auto& descriptor_set : descriptor_sets) {
            write_buff_desc_sets.push_back(vk::WriteDescriptorSet()
                .setDstSet(descriptor_set)
                .setDstBinding((int) PerFrameBufferBindings::LightPropertiesBuffer)
            );
        }

        light_memory.init(
            frame_overlap,
            sizeof(Light::Properties),
            initial_capacity,
            write_buff_desc_sets.data(),
            allocator,
            ctx
        );
    }

    void LightManager::destroy() {
        light_memory.destroy();
        lights.clear();
        descriptor_sets.clear();
    }

    bool LightManager::add_light(std::shared_ptr<Light> light) {
        return lights.insert(light).second;
    }

    bool LightManager::remove_light(std::shared_ptr<Light> light) {
        return lights.erase(light);
    }

    void LightManager::update(uint safe_frame) {
        light_memory.reserve(lights.size(), safe_frame);
        size_t i = 0;
        for (auto& light : lights) {
            Light::Properties light_properties = light->get_properties();
            light_memory.add_object_direct(i, &light_properties, safe_frame);
            ++i;
        }
    }


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