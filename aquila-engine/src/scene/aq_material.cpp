#include "scene/aq_material.hpp"

#include <list>
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
        this->allocator = allocator;
        this->ctx = upload_context;

        buffer_datas.resize(frame_overlap);

        vk::DeviceSize allocation_size = nr_materials * sizeof(Material::Properties);
        for (auto& buffer_data : buffer_datas) {
            buffer_data.material_buffer.allocate(allocator, allocation_size, vk::BufferUsageFlagBits::eStorageBuffer, vma::MemoryUsage::eCpuToGpu);

            auto[mm_res, buff_mem] = allocator->mapMemory(buffer_data.material_buffer.get_allocation());
            CHECK_VK_RESULT_R(mm_res, false, "Failed to map material buffer memory");
            buffer_data.p_mat_buff_mem = (unsigned char*)buff_mem;
        }

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

        return true;
    }

    void MaterialManager::add_material(std::shared_ptr<Material> material) {
        // Material was already added
        if (material_indices.find(material) != material_indices.end())
            return;

        uint material_index = material_datas.size();
        material_datas.push_back({material, {}});
        updated_materials.push_back(material_index);
        material_indices[material] = material_index;

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
    }

    bool MaterialManager::update_material(std::shared_ptr<Material> material) {
        auto it = material_indices.find(material);
        if (it == material_indices.end()) return false;
        uint material_index = it->second;

        material_datas[material_index].uploaded_buffers.clear();

        // Material is not in `updated_materials` because it was already fully uploaded
        if (material_datas[material_index].uploaded_buffers.size() >= buffer_datas.size())
            updated_materials.push_back(material_index);


        return true;
    }

    uint MaterialManager::add_texture(std::shared_ptr<Texture> texture) {
        auto tex_it = texture_indices.find(texture->get_path());
        if (tex_it != texture_indices.end()) { // Texture was already added
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
        // Resize buffer if needed

        vk::DeviceSize min_buffer_size = material_datas.size() * sizeof(Material::Properties);
        if (buffer_datas[safe_frame].material_buffer.get_size() < min_buffer_size) {
            // A larger `nr_materials` might already have been calculated by a previous frame
            if (nr_materials < material_datas.size()) 
                nr_materials = material_datas.size() * 3 / 2;

            vk::DeviceSize new_buffer_size = nr_materials * sizeof(Material::Properties);
            buffer_datas[safe_frame].material_buffer.resize(new_buffer_size, ctx);

            auto[mm_res, buff_mem] = allocator->mapMemory(buffer_datas[safe_frame].material_buffer.get_allocation());
            CHECK_VK_RESULT(mm_res, "Failed to map material buffer memory");
            buffer_datas[safe_frame].p_mat_buff_mem = (unsigned char*)buff_mem;

            std::array<vk::DescriptorBufferInfo, 1> buffer_infos{{
                { buffer_datas[safe_frame].material_buffer.get_buffer(), 0, nr_materials * sizeof(Material::Properties) }
            }};
            vk::WriteDescriptorSet write_buff_desc_set = vk::WriteDescriptorSet()
                .setDstSet(buffer_datas[safe_frame].desc_set)
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setBufferInfo(buffer_infos);

            ctx.device.updateDescriptorSets({write_buff_desc_set}, {});
        }

        // Update buffers

        auto it = updated_materials.begin();
        while (it != updated_materials.end()) {
            std::shared_ptr<Material> mat = material_datas[*it].material;
            auto& uploaded_buffers = material_datas[*it].uploaded_buffers;

            if (std::find(std::begin(uploaded_buffers), std::end(uploaded_buffers), safe_frame) == std::end(uploaded_buffers)) { // Check if the material still needs updating on this frame
                uploaded_buffers.push_back(safe_frame);

                vk::DeviceSize offset = (*it) * sizeof(Material::Properties);
                memcpy(buffer_datas[safe_frame].p_mat_buff_mem + offset, &mat->properties, sizeof(Material::Properties));
            }

            // Go to the next material index in need of updating
            if (uploaded_buffers.size() >= buffer_datas.size())
                it = updated_materials.erase(it);
            else
                ++it;
        }

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
                    .setDstSet(buffer_datas[safe_frame].desc_set)
                    .setDstBinding(2)
                    .setDstArrayElement(it2->first)
                    .setDescriptorType(vk::DescriptorType::eSampledImage)
                    .setDescriptorCount(1)
                    .setPImageInfo(&image_infos.back());

                descriptor_writes.push_back(write_tex_desc_set);
            }

            if (it2->second.size() >= buffer_datas.size()) {
                it2 = added_textures.erase(it2);
            } else {
                ++it2;
            }
        }

        ctx.device.updateDescriptorSets(descriptor_writes, {});

    }

    vk::DeviceSize MaterialManager::get_buffer_offset(uint frame) {
        return frame * nr_materials * sizeof(Material::Properties);
    }

    uint MaterialManager::get_material_index(std::shared_ptr<Material> material) {
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

    void MaterialManager::create_descriptor_set() {
        // Descriptor pool

        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes{{
            {vk::DescriptorType::eStorageBuffer, (uint32_t) buffer_datas.size()},
            {vk::DescriptorType::eSampler, (uint32_t) buffer_datas.size()},
            {vk::DescriptorType::eSampledImage, uint32_t(max_nr_textures * buffer_datas.size())},
        }};
        vk::DescriptorPoolCreateInfo desc_pool_create_info({}, (max_nr_textures + 2) * buffer_datas.size(), descriptor_pool_sizes);

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
        desc_set_layouts.reserve(buffer_datas.size());
        for (uint i=0; i<buffer_datas.size(); ++i) { desc_set_layouts.push_back(desc_set_layout); }
        vk::DescriptorSetAllocateInfo mat_desc_set_alloc_info(desc_pool, buffer_datas.size(), desc_set_layouts.data());

        auto[atds_res, desc_sets] = ctx.device.allocateDescriptorSets(mat_desc_set_alloc_info);
        CHECK_VK_RESULT(atds_res, "Failed to allocate descriptor sets");

        for (uint i=0; i<buffer_datas.size(); ++i) {
            buffer_datas[i].desc_set = desc_sets[i];

            std::array<vk::DescriptorBufferInfo, 1> buffer_infos{{
                { buffer_datas[i].material_buffer.get_buffer(), 0, nr_materials * sizeof(Material::Properties) }
            }};
            vk::WriteDescriptorSet write_buff_desc_set = vk::WriteDescriptorSet()
                .setDstSet(desc_sets[i])
                .setDescriptorType(vk::DescriptorType::eStorageBuffer)
                .setBufferInfo(buffer_infos);

            std::array<vk::DescriptorImageInfo, 1> sampler_infos{{
                {default_sampler, nullptr, vk::ImageLayout::eShaderReadOnlyOptimal}
            }};
            vk::WriteDescriptorSet write_samp_desc_set = vk::WriteDescriptorSet()
                .setDstSet(desc_sets[i])
                .setDstBinding(1)
                .setDescriptorType(vk::DescriptorType::eSampler)
                .setImageInfo(sampler_infos);

            std::vector<vk::DescriptorImageInfo> image_infos;
            image_infos.reserve(max_nr_textures);
            for (size_t i=0; i<max_nr_textures; ++i)
                image_infos.push_back(textures[0]->get_image_info());

            vk::WriteDescriptorSet write_tex_desc_set = vk::WriteDescriptorSet()
                .setDstSet(desc_sets[i])
                .setDstBinding(2)
                .setDescriptorType(vk::DescriptorType::eSampledImage)
                .setImageInfo(image_infos);

            ctx.device.updateDescriptorSets({write_buff_desc_set, write_samp_desc_set, write_tex_desc_set}, {});
        }
    }

    void MaterialManager::destroy() {
        ctx.device.destroyDescriptorSetLayout(desc_set_layout);
        ctx.device.destroySampler(default_sampler);
        ctx.device.destroyDescriptorPool(desc_pool);
        // material_buffer.destroy();
        for (auto& buffer_data : buffer_datas) {
            buffer_data.material_buffer.destroy();
        }

        material_datas.clear();
        material_indices.clear();
        updated_materials.clear();

        // Clearing textures vector will free unused textures
        textures.clear();
        added_textures.clear();
    }

}