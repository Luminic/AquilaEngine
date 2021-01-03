#include "scene/aq_material.hpp"

#include <algorithm>
#include <iostream>

#include "util/vk_utility.hpp"

namespace aq {

    Material::Material() {}

    Material::~Material() {}


    MaterialManager::MaterialManager() {
        // Default (error) material
        default_material = std::make_shared<Material>();
        default_material->properties.albedo = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f);
        add_material(default_material); // guaranteed to be first
    }

    bool MaterialManager::init(size_t nr_materials, uint frame_overlap, vma::Allocator* allocator, vk_util::UploadContext upload_context) {
        this->nr_materials = nr_materials;
        this->frame_overlap = frame_overlap;
        this->allocator = allocator;
        this->ctx = upload_context;

        allocation_size = frame_overlap * nr_materials * sizeof(Material::Properties);
        material_buffer.allocate(allocator, allocation_size, vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eCpuToGpu);

        auto[mm_res, buff_mem] = allocator->mapMemory(material_buffer.allocation);
        CHECK_VK_RESULT_R(mm_res, false, "Failed to map material buffer memory");
        p_mat_buff_mem = (unsigned char*)buff_mem;

        // Create Sampler for textures

        vk::SamplerCreateInfo sampler_create_info(
            {}, // flags
            vk::Filter::eLinear,
            vk::Filter::eLinear,
            vk::SamplerMipmapMode::eNearest,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            vk::SamplerAddressMode::eClampToEdge,
            0.0f, // mip lod bias
            VK_FALSE, // anisotropy enable
            0.0f // max anisotropy
        );

        vk::Result cs_result;
        std::tie(cs_result, default_sampler) = ctx.device.createSampler(sampler_create_info);
        CHECK_VK_RESULT_R(cs_result, false, "Failed to create sampler");

        // Placeholder texture

        if (!placeholder_texture.upload_from_file("/home/l/C++/Vulkan/Vulkan-Engine/sandbox/resources/happy-tree.png", allocator, ctx)) return false;

        return true;
    }

    void MaterialManager::add_material(std::shared_ptr<Material> material) {
        material->manager = this;
        material->material_index = materials.size();

        materials.push_back(material);

        updated_materials.push_back({material, {}});
    }

    void MaterialManager::update(uint safe_frame) {
        auto it=updated_materials.begin();
        while (it != updated_materials.end()) {
            if (std::find(std::begin(it->second), std::end(it->second), safe_frame) == std::end(it->second))
                it->second.push_back(safe_frame);
    
            std::shared_ptr<Material> mat = it->first.lock();
            if (mat) {
                vk::DeviceSize offset = get_buffer_offset(safe_frame);
                offset += mat->material_index * sizeof(Material::Properties);
                memcpy(p_mat_buff_mem + offset, &mat->properties, sizeof(Material::Properties));
            }

            if (it->second.size() >= frame_overlap || it->first.expired()) {
                it = updated_materials.erase(it);
            } else {
                ++it;
            }
        }
    }

    vk::DeviceSize MaterialManager::get_buffer_offset(uint frame) {
        return frame * nr_materials * sizeof(Material::Properties);
    }

    uint MaterialManager::get_material_index(std::shared_ptr<Material> material, uint frame) {
        if (material && material->manager == this)
            return frame * nr_materials + material->material_index;
        return frame * nr_materials;
    }

    void MaterialManager::create_descriptor_set(vk::DescriptorPool desc_pool) {
        // Descriptor set layout

        std::array<vk::DescriptorSetLayoutBinding, 2> bindings{{
            { 0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment },
            { 1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment }
        }};
        vk::DescriptorSetLayoutCreateInfo desc_set_create_info({}, bindings);

        vk::Result ctds_res;
        std::tie(ctds_res, desc_set_layout) = ctx.device.createDescriptorSetLayout(desc_set_create_info);
        CHECK_VK_RESULT(ctds_res, "Failed to create texture set layout");

        // Descriptor sets

        vk::DescriptorSetAllocateInfo mat_desc_set_alloc_info(desc_pool, 1, &desc_set_layout);

        auto[atds_res, mat_desc_sets] = ctx.device.allocateDescriptorSets(mat_desc_set_alloc_info);
        CHECK_VK_RESULT(atds_res, "Failed to allocate descriptor set");
        desc_set = mat_desc_sets[0];

        std::array<vk::DescriptorImageInfo, 1> image_infos{{
            placeholder_texture.get_image_info(default_sampler)
        }};
        vk::WriteDescriptorSet write_tex_desc_set(
            desc_set, // dst set
            0, // dst binding
            0, // dst array element
            vk::DescriptorType::eCombinedImageSampler,
            image_infos, // image infos
            {} // buffer infos
        );

        std::array<vk::DescriptorBufferInfo, 1> buffer_infos{{
            {material_buffer.buffer, 0, allocation_size}
        }};
        vk::WriteDescriptorSet write_buff_desc_set(
            desc_set, // dst set
            1, // dst binding
            0, // dst array element
            vk::DescriptorType::eStorageBuffer,
            {}, // image infos
            buffer_infos // buffer infos
        );
        ctx.device.updateDescriptorSets({write_tex_desc_set, write_buff_desc_set}, {});
    }

    void MaterialManager::destroy() {
        ctx.device.destroyDescriptorSetLayout(desc_set_layout);
        ctx.device.destroySampler(default_sampler);
        material_buffer.destroy();
        placeholder_texture.destroy();
    }

}