#ifndef SCENE_AQUILA_MATERIAL_HPP
#define SCENE_AQUILA_MATERIAL_HPP

#include <vector>
#include <list>
#include <unordered_map>
#include <memory>
#include <utility>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "util/vk_resizable_buffer.hpp"
#include "util/aq_memory_manager.hpp"
#include "scene/aq_texture.hpp"

namespace aq {

    class MaterialManager;

    class Material {
    public:
        Material();
        ~Material();

        struct Properties {
            glm::vec3 albedo = glm::vec3(1.0f);
            uint albedo_ti = 0;

            float roughness = 0.0f;
            uint roughness_ti = 0;
            float metalness = 0.0f;
            uint metalness_ti = 0;

            glm::vec3 ambient = glm::vec3(0.1f);
            uint ambient_occlusion_ti = 0;

            uint normal_ti = 0;
            float fdata0;
            int idata0;
            int idata1;
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
            uint max_nr_textures,
            uint frame_overlap, 
            vma::Allocator* allocator, 
            vk_util::UploadContext upload_context
        );

        // Destroy material buffer, descriptor layout, and descriptor sets
        void destroy();

        // Material memory will not actually be uploaded to the GPU until `update` is called
        void add_material(std::shared_ptr<Material> material);
        // Returns false if material is not being managed (has not been added)
        bool update_material(std::shared_ptr<Material> material);

        // If the texture has not been uploaded, will upload texture from its path
        // Returns texture index
        uint add_texture(std::shared_ptr<Texture> texture);

        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        // Uploads material memory to the buffer for `safe_frame`
        void update(uint safe_frame);

        // Gets the offset for the virtual buffer for `frame`
        vk::DeviceSize get_buffer_offset(uint frame);
        // Gets the offset for the virtual buffer for `material` on `frame`
        // A null `material` or a material not managed by `this` will return material 0
        uint get_material_index(std::shared_ptr<Material> material);

        // Returns a texture if `MaterialManager` has a texture with path `path`
        // Otherwise returns null
        std::shared_ptr<Texture> get_texture(std::string path);

        vk::DescriptorSetLayout get_descriptor_set_layout() { return desc_set_layout; };
        // vk::DescriptorSet get_descriptor_set(uint frame) { return buffer_datas[frame].desc_set; };
        vk::DescriptorSet get_descriptor_set(uint frame) { return descriptor_sets[frame]; };

        std::shared_ptr<Material> default_material;

    private:
        // struct BufferData {
        //     ResizableBuffer material_buffer;
        //     unsigned char* p_mat_buff_mem;
        //     vk::DescriptorSet desc_set;
        // };
        // std::vector<BufferData> buffer_datas;
        std::vector<vk::DescriptorSet> descriptor_sets;
        void create_descriptor_sets();

        MemoryManager material_memory;

        // struct MaterialData {
        //     std::shared_ptr<Material> material;
        //     std::vector<uint> uploaded_buffers;
        // };
        // std::vector<MaterialData> material_datas;
        // When materials are pushed into `materials` their index is added to `updated_materials`
        // When `update` is called, `safe_frame` is added to the uint vector for each material in `updated_materials`
        // Once the uint vector contains all the frames (size == `frame_overlap`) the material had been fully uploaded and is
        // removed from the list
        // std::list<uint> updated_materials;
        std::unordered_map<std::shared_ptr<Material>, ManagedMemoryIndex> material_indices;

        std::vector<std::shared_ptr<Texture>> textures;
        // Similar to `updated_materials` but the vector of frames the texture descriptor has been uploaded to is stored directly in the list
        std::list<std::pair<uint, std::vector<uint>>> added_textures;
        // Texture path to texture index
        std::unordered_map<std::string, uint> texture_indices;

        size_t nr_materials;
        uint max_nr_textures;

        vma::Allocator* allocator;
        vk_util::UploadContext ctx;

        vk::Sampler default_sampler;

        vk::DescriptorPool desc_pool; // Separate pool for material data
        vk::DescriptorSetLayout desc_set_layout;
    };

}

#endif