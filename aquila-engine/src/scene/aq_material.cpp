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

    bool MaterialManager::init(size_t nr_materials, uint max_nr_textures, uint frame_overlap, vma::Allocator* allocator, vk_util::UploadContext upload_context) {
        this->nr_materials = nr_materials;
        this->max_nr_textures = max_nr_textures;
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
        std::string placeholder_texture_path = std::string(AQUILA_ENGINE_PATH) + "/resources/missing_texture.png";
        if (!placeholder_texture.upload_from_file(placeholder_texture_path.c_str(), allocator, ctx)) return false;

        return true;
    }

    void MaterialManager::add_material(std::shared_ptr<Material> material) {
        if (material->manager)
            std::cerr << "A material can only be managed by one manager at a time." << std::endl;
        material->manager = this;
        material->material_index = materials.size();
        material->uploaded_buffers.clear();

        materials.push_back(material);

        updated_materials.push_back(material);
    }

    void MaterialManager::update_material(std::shared_ptr<Material> material) {
        assert(material->manager == this);
        if (material->uploaded_buffers.size() >= frame_overlap) {
            updated_materials.push_back(material);
        }
        material->uploaded_buffers.clear();
    }

    void MaterialManager::update(uint safe_frame) {
        auto it=updated_materials.begin();
        while (it != updated_materials.end()) {
            std::shared_ptr<Material> mat = it->lock();
            if (mat) {
                if (std::find(std::begin(mat->uploaded_buffers), std::end(mat->uploaded_buffers), safe_frame) == std::end(mat->uploaded_buffers))
                    mat->uploaded_buffers.push_back(safe_frame);

                vk::DeviceSize offset = get_buffer_offset(safe_frame);
                offset += mat->material_index * sizeof(Material::Properties);
                memcpy(p_mat_buff_mem + offset, &mat->properties, sizeof(Material::Properties));

                if (mat->uploaded_buffers.size() >= frame_overlap)
                    it = updated_materials.erase(it);
                else
                    ++it;

            } else {
                it = updated_materials.erase(it);
            }
        }
    }

    vk::DeviceSize MaterialManager::get_buffer_offset(uint frame) {
        return frame * nr_materials * sizeof(Material::Properties);
    }

    uint MaterialManager::get_material_index(std::shared_ptr<Material> material) {
        if (material && material->manager == this)
            return material->material_index;
        return 0;
    }

    void MaterialManager::create_descriptor_set() {
        // Descriptor pool

        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes{{
            {vk::DescriptorType::eStorageBuffer, frame_overlap},
            {vk::DescriptorType::eSampler, frame_overlap},
            {vk::DescriptorType::eSampledImage, max_nr_textures * frame_overlap},
        }};
        vk::DescriptorPoolCreateInfo desc_pool_create_info({}, (max_nr_textures + 2) * frame_overlap, descriptor_pool_sizes);

        vk::Result cdp_result;
        std::tie(cdp_result, desc_pool) = ctx.device.createDescriptorPool(desc_pool_create_info);
        CHECK_VK_RESULT(cdp_result, "Failed to create descriptor pool");

        // Descriptor set layout

        std::array<vk::DescriptorSetLayoutBinding, 3> bindings{{
            { 0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eFragment },
            { 1, vk::DescriptorType::eSampler, 1, vk::ShaderStageFlagBits::eFragment },
            { 2, vk::DescriptorType::eSampledImage, max_nr_textures, vk::ShaderStageFlagBits::eFragment }
        }};
        vk::DescriptorSetLayoutCreateInfo desc_set_create_info({}, bindings);

        vk::Result ctds_res;
        std::tie(ctds_res, desc_set_layout) = ctx.device.createDescriptorSetLayout(desc_set_create_info);
        CHECK_VK_RESULT(ctds_res, "Failed to create texture set layout");

        // Descriptor sets

        std::vector<vk::DescriptorSetLayout> desc_set_layouts;
        desc_set_layouts.reserve(frame_overlap);
        for (uint i=0; i<frame_overlap; ++i) { desc_set_layouts.push_back(desc_set_layout); }
        vk::DescriptorSetAllocateInfo mat_desc_set_alloc_info(desc_pool, frame_overlap, desc_set_layouts.data());

        vk::Result atds_res;
        std::tie(atds_res, desc_sets) = ctx.device.allocateDescriptorSets(mat_desc_set_alloc_info);
        CHECK_VK_RESULT(atds_res, "Failed to allocate descriptor set");

        for (uint i=0; i<frame_overlap; ++i) {
            std::array<vk::DescriptorBufferInfo, 1> buffer_infos{{
                {
                    material_buffer.buffer, 
                    i * nr_materials * sizeof(Material::Properties), // offset
                    nr_materials * sizeof(Material::Properties) // size
                }
            }};
            vk::WriteDescriptorSet write_buff_desc_set(
                desc_sets[i], // dst set
                0, // dst binding
                0, // dst array element
                vk::DescriptorType::eStorageBuffer,
                {}, // image infos
                buffer_infos // buffer infos
            );

            std::array<vk::DescriptorImageInfo, 1> sampler_infos{{
                {default_sampler, nullptr, vk::ImageLayout::eShaderReadOnlyOptimal}
            }};
            vk::WriteDescriptorSet write_samp_desc_set(
                desc_sets[i], // dst set
                1, // dst binding
                0, // dst array element
                vk::DescriptorType::eSampler,
                sampler_infos, // image infos
                {} // buffer infos
            );

            std::vector<vk::DescriptorImageInfo> image_infos;
            image_infos.reserve(max_nr_textures);
            for (size_t i=0; i<max_nr_textures; ++i)
                image_infos.push_back(placeholder_texture.get_image_info());

            vk::WriteDescriptorSet write_tex_desc_set(
                desc_sets[i], // dst set
                2, // dst binding
                0, // dst array element
                vk::DescriptorType::eSampledImage,
                image_infos, // image infos
                {} // buffer infos
            );

            ctx.device.updateDescriptorSets({write_buff_desc_set, write_samp_desc_set, write_tex_desc_set}, {});
        }
    }

    void MaterialManager::destroy() {
        ctx.device.destroyDescriptorSetLayout(desc_set_layout);
        ctx.device.destroySampler(default_sampler);
        ctx.device.destroyDescriptorPool(desc_pool);
        material_buffer.destroy();
        placeholder_texture.destroy();
    }

}