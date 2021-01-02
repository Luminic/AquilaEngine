#ifndef SCENE_AQUILA_MATERIAL_HPP
#define SCENE_AQUILA_MATERIAL_HPP

#include <vector>
#include <list>
#include <set>
#include <memory>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "scene/aq_texture.hpp"


namespace aq {

    class MaterialManager;

    class Material {
    public:
        Material();
        ~Material();

        struct Properties {
            // w is whether or not to use the corresponding texture
            glm::vec4 albedo; 
            glm::vec4 roughness;
            glm::vec4 metalness;
            glm::vec4 ambient_occlusion;
        };
        Properties properties;

        std::array<std::shared_ptr<Texture>, 5> textures{};
        enum TextureType {
            Albedo = 0,
            Roughness = 1,
            Metalness = 2,
            AmbientOcclusion = 3,
            Normal = 4
        };

    private:
        MaterialManager* manager; // set by `MaterialManager`
        size_t material_index; // set by `MaterialManager`

        friend class MaterialManager;
    };

    class MaterialManager {
    public:
        MaterialManager();

        // `nr_materials` is the number of materials `MaterialManager` should be able
        // to handle
        //
        // `frame_overlap` is the number of copies MaterialManager will have of the material
        // data so material data can be updated without witing for the GPU to finish
        // rendering all frames
        bool init(
            size_t nr_materials, 
            uint frame_overlap, 
            vk::DeviceSize min_ubo_alignment, 
            vma::Allocator* allocator, 
            vk_util::UploadContext upload_context
        );

        // Material memory will not actually be uploaded to the GPU until `update` is called
        void add_material(std::shared_ptr<Material> material);

        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        // Uploads material memory to the buffer for `safe_frame`
        void update(uint safe_frame);

        // Gets the offset for the virtual buffer for `frame`
        vk::DeviceSize get_buffer_offset(uint frame);
        // Gets the offset for the virtual buffer for `material` on `frame`
        vk::DeviceSize get_material_offset(std::shared_ptr<Material> material, uint frame);

        void create_descriptor_set(vk::DescriptorPool desc_pool);

        // Destroy material buffer, descriptor layout, and descriptor sets
        void destroy();

        vk::DescriptorSetLayout get_descriptor_set_layout() { return desc_set_layout; };
        vk::DescriptorSet get_descriptor_set() { return desc_set; };

    private:
        AllocatedBuffer material_buffer; // Has `allocation_size` memory
        unsigned char* p_mat_buff_mem;
        vk::DeviceSize allocation_size; // calculated as `frame_overlap * nr_materials * pad_uniform_buffer_size(sizeof(Material), min_ubo_alignment)`

        std::vector<std::weak_ptr<Material>> materials;
        // Materials are pushed into both `updated_materials` and `materials` when added
        // When `update` is called, `safe_frame` is added to the set for each material in `updated_materials`
        // Once the set contains all the frames (size = `frame_overlap`) the material had been fully uploaded and is
        // removed from the list
        std::list<std::pair<std::weak_ptr<Material>, std::set<uint>>> updated_materials;

        size_t nr_materials;
        uint frame_overlap;
        vk::DeviceSize min_ubo_alignment;
        vma::Allocator* allocator;
        vk_util::UploadContext ctx;

        vk::Sampler default_sampler;
        Texture placeholder_texture;

        vk::DescriptorSetLayout desc_set_layout;
        vk::DescriptorSet desc_set;
    };

}

#endif