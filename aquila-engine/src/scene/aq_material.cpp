#include "scene/aq_material.hpp"

#include <list>
#include <algorithm>
#include <iostream>

#include "util/vk_utility.hpp"

namespace aq {

    Material::Material() {}
    Material::Material(const std::string& name) : name(name) {}

    Material::~Material() {}


    MaterialManager::MaterialManager() {
        // Default (error) material
        default_material = std::make_shared<Material>("Error Material");
        default_material->properties.albedo = glm::vec4(1.0f, 0.0f, 1.0f, 0.0f);
    }

    bool MaterialManager::init(size_t nr_materials, uint max_nr_textures, uint frame_overlap, vma::Allocator* allocator, vk_util::UploadContext upload_context) {
        this->nr_materials = nr_materials;
        this->max_nr_textures = max_nr_textures;
        this->allocator = allocator;
        this->ctx = upload_context;

        descriptor_sets.resize(frame_overlap);

        // Create Sampler for textures
        vk::SamplerCreateInfo sampler_create_info = vk::SamplerCreateInfo()
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eNearest)
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setAddressModeW(vk::SamplerAddressMode::eRepeat)
            .setMipLodBias(0.0f)
            .setAnisotropyEnable(VK_FALSE);

        vk::Result cs_result;
        std::tie(cs_result, default_sampler) = ctx.device.createSampler(sampler_create_info);
        CHECK_VK_RESULT_R(cs_result, false, "Failed to create sampler");

        // Placeholder texture
        std::string placeholder_texture_path = std::string(AQUILA_ENGINE_PATH) + "/resources/missing_texture.png";
        textures.push_back(std::make_shared<Texture>());
        assert(textures.size() == 1); // Error texture must be the first texture
        if (!textures[0]->upload_from_file(placeholder_texture_path, allocator, ctx)) return false;

        // Init material buffers

        create_descriptor_sets();

        std::vector<vk::WriteDescriptorSet> write_buff_desc_sets;
        for (auto& desc_set : descriptor_sets) {
            write_buff_desc_sets.push_back(vk::WriteDescriptorSet().setDstSet(desc_set));
        }

        material_memory.init(
            frame_overlap,
            sizeof(Material::Properties),
            50,
            write_buff_desc_sets.data(),
            allocator,
            ctx
        );

        add_material(default_material);

        return true;
    }

    void MaterialManager::add_material(std::shared_ptr<Material> material) {
        // Material was already added
        if (material_indices.find(material) != material_indices.end())
            return;

        if (material->textures[Material::Albedo]) {
            material->properties.albedo_ti = add_texture(material->textures[Material::Albedo]);
        }
        if (material->textures[Material::Roughness]) {
            material->properties.roughness_ti = add_texture(material->textures[Material::Roughness]);
        }
        if (material->textures[Material::Metalness]) {
            material->properties.metalness_ti = add_texture(material->textures[Material::Metalness]);
        }
        if (material->textures[Material::AmbientOcclusion]) {
            material->properties.ambient_occlusion_ti = add_texture(material->textures[Material::AmbientOcclusion]);
        }
        if (material->textures[Material::Normal]) {
            material->properties.normal_ti = add_texture(material->textures[Material::Normal]);
        }

        ManagedMemoryIndex material_index = material_memory.add_object((void*) &material->properties);
        material_indices[material] = material_index;
    }

    bool MaterialManager::update_material(std::shared_ptr<Material> material) {
        auto it = material_indices.find(material);
        if (it == material_indices.end()) return false;
        ManagedMemoryIndex material_index = it->second;

        material_memory.update_object(material_index, (void*) &material->properties);

        return true;
    }

    bool MaterialManager::remove_material(std::shared_ptr<Material> material) {
        auto it = material_indices.find(material);
        if (it == material_indices.end()) return false;
        ManagedMemoryIndex material_index = it->second;
        material_indices.erase(it);

        material_memory.remove_object(material_index);

        return true;
    }

    uint MaterialManager::add_texture(std::shared_ptr<Texture> texture) {
        auto tex_it = texture_indices.find(texture->get_path());
        if (tex_it != texture_indices.end()) { // Texture was already added
            texture->clear_cpu_data();
            return tex_it->second; 
        }

        if (!texture->is_uploaded()) {
            if (!texture->upload(allocator, ctx)) {
                return 0;
            }
        }

        uint texture_index = (uint)textures.size();

        textures.push_back(texture);
        texture_indices.insert({texture->get_path(), texture_index});
        added_textures.push_back({texture_index, {}});

        return texture_index;
    }

    void MaterialManager::update(uint safe_frame) {
        material_memory.update(safe_frame);

        std::vector<vk::WriteDescriptorSet> descriptor_writes;
        // Needed to store the memory for `descriptor_writes` because `vk::WriteDescriptorSet` only keeps a pointer
        // to `vk::DescriptorImageInfo`. A list is necessary so pointers aren't invalidated
        std::list<vk::DescriptorImageInfo> image_infos;

        auto it2 = added_textures.begin();
        while (it2 != added_textures.end()) {
            if (std::find(std::begin(it2->second), std::end(it2->second), safe_frame) == std::end(it2->second)) {
                it2->second.push_back(safe_frame);

                image_infos.push_back(textures[it2->first]->get_image_info());

                vk::WriteDescriptorSet write_tex_desc_set = vk::WriteDescriptorSet()
                    .setDstSet(descriptor_sets[safe_frame])
                    .setDstBinding(2)
                    .setDstArrayElement(it2->first)
                    .setDescriptorType(vk::DescriptorType::eSampledImage)
                    .setDescriptorCount(1)
                    .setPImageInfo(&image_infos.back());

                descriptor_writes.push_back(write_tex_desc_set);
            }

            if (it2->second.size() >= descriptor_sets.size()) {
                it2 = added_textures.erase(it2);
            } else {
                ++it2;
            }
        }

        ctx.device.updateDescriptorSets(descriptor_writes, {});
    }

    ManagedMemoryIndex MaterialManager::get_material_index(std::shared_ptr<Material> material) {
        auto it = material_indices.find(material);
        if (it == material_indices.end()) return 0; // Material not found so return default material
        return it->second;
    }

    std::shared_ptr<Texture> MaterialManager::get_texture(std::string path) {
        auto texture_index = texture_indices.find(path);
        if (texture_index != texture_indices.end())
            return textures[texture_index->second];
        return {};
    }

    void MaterialManager::create_descriptor_sets() {
        // Descriptor pool

        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes{{
            {vk::DescriptorType::eStorageBuffer, (uint32_t) descriptor_sets.size()},
            {vk::DescriptorType::eSampler, (uint32_t) descriptor_sets.size()},
            {vk::DescriptorType::eSampledImage, uint32_t(max_nr_textures * descriptor_sets.size())},
        }};
        vk::DescriptorPoolCreateInfo desc_pool_create_info({}, (max_nr_textures + 2) * descriptor_sets.size(), descriptor_pool_sizes);

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
        desc_set_layouts.reserve(descriptor_sets.size());
        for (uint i=0; i<descriptor_sets.size(); ++i) { desc_set_layouts.push_back(desc_set_layout); }
        vk::DescriptorSetAllocateInfo mat_desc_set_alloc_info(desc_pool, descriptor_sets.size(), desc_set_layouts.data());

        vk::Result atds_res = ctx.device.allocateDescriptorSets(&mat_desc_set_alloc_info, descriptor_sets.data());
        CHECK_VK_RESULT(atds_res, "Failed to allocate descriptor sets");

        for (uint i=0; i<descriptor_sets.size(); ++i) {
            std::array<vk::DescriptorImageInfo, 1> sampler_infos{{
                {default_sampler, nullptr, vk::ImageLayout::eShaderReadOnlyOptimal}
            }};
            vk::WriteDescriptorSet write_samp_desc_set = vk::WriteDescriptorSet()
                .setDstSet(descriptor_sets[i])
                .setDstBinding(1)
                .setDescriptorType(vk::DescriptorType::eSampler)
                .setImageInfo(sampler_infos);

            std::vector<vk::DescriptorImageInfo> image_infos;
            image_infos.reserve(max_nr_textures);
            for (size_t i=0; i<max_nr_textures; ++i)
                image_infos.push_back(textures[0]->get_image_info());

            vk::WriteDescriptorSet write_tex_desc_set = vk::WriteDescriptorSet()
                .setDstSet(descriptor_sets[i])
                .setDstBinding(2)
                .setDescriptorType(vk::DescriptorType::eSampledImage)
                .setImageInfo(image_infos);

            ctx.device.updateDescriptorSets({write_samp_desc_set, write_tex_desc_set}, {});
        }
    }

    void MaterialManager::destroy() {
        ctx.device.destroyDescriptorSetLayout(desc_set_layout);
        ctx.device.destroySampler(default_sampler);
        ctx.device.destroyDescriptorPool(desc_pool);

        material_memory.destroy();
        descriptor_sets.clear();

        material_indices.clear();

        // Clearing textures vector will free unused textures
        textures.clear();
        added_textures.clear();
    }

}