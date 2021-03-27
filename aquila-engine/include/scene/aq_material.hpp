#ifndef SCENE_AQUILA_MATERIAL_HPP
#define SCENE_AQUILA_MATERIAL_HPP

#include <memory>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <utility>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "util/vk_resizable_buffer.hpp"
#include "util/vk_descriptor_set_builder.hpp"
#include "util/vk_memory_manager_retained.hpp"
#include "scene/aq_texture.hpp"

namespace aq {

    class Material {
    public:
        Material();
        Material(const std::string& name);
        ~Material();

        struct Properties {
            glm::vec3 albedo = glm::vec3(1.0f);
            uint albedo_ti = 0;

            float roughness = 0.0f;
            uint roughness_ti = 0;
            float metalness = 0.0f;
            uint metalness_ti = 0;

            glm::vec3 ambient = glm::vec3(0.005f);
            uint ambient_occlusion_ti = 0;

            uint normal_ti = 0;
            float fdata0;
            int idata0;
            int idata1;
        };
        Properties properties;

        enum TextureType {
            Albedo = 0,
            Roughness = 1,
            Metalness = 2,
            AmbientOcclusion = 3,
            Normal = 4
        };
        std::array<std::shared_ptr<Texture>, 5> textures{};

        // `name` should never be used as an ID; it's just an easy way for users to identify materials
        std::string name;
    };

    class MaterialManager {
    public:
        MaterialManager();

        // Initialize the `MaterialManager`
        // Call `descriptor_sets_created(...)` after `per_frame_descriptor_set_builder.build()`
        void init(
            uint frame_overlap,
            size_t texture_capacity, // texture capacity is fixed
            size_t initial_material_capacity, // material capacity can grow as needed
            DescriptorSetBuilder& per_frame_descriptor_set_builder, // should have multiplicity of `frame_overlap`
            vma::Allocator* allocator,
            vk_util::UploadContext upload_context
        );

        // Call after `per_frame_descriptor_set_builder.build()` where `per_frame_descriptor_set_builder` is the same one from `init`
        // Will update the descriptor sets
        // `descriptor_sets` should have size `frame_overlap` 
        void descriptor_sets_created(const std::vector<vk::DescriptorSet>& descriptor_sets);

        // Destroy material buffer and other resources
        void destroy();

        // Returns false if material is not being managed (has not been added)
        bool add_material(std::shared_ptr<Material> material); // Material memory will not actually be uploaded to the GPU until `update` is called
        bool update_material(std::shared_ptr<Material> material);
        bool remove_material(std::shared_ptr<Material> material);

        // If the texture has not been uploaded, will upload texture from its path
        // Returns texture index
        uint add_texture(std::shared_ptr<Texture> texture);

        // `safe_frame` must be finished rendering (usually the frame about to be rendered onto)
        // Uploads material memory to the buffer for `safe_frame`
        void update(uint safe_frame);

        // Gets the index for `material` in the buffer
        // An unknown material will return index 0 (error material)
        ManagedMemoryIndex get_material_index(std::shared_ptr<Material> material);

        // Returns a texture if `MaterialManager` has a texture with path `path`
        // Otherwise returns null
        std::shared_ptr<Texture> get_texture(std::string path);

        std::shared_ptr<Material> default_material;

    private:
        // std::vector<vk::DescriptorSet> descriptor_sets;
        void create_descriptor_writes();

        MemoryManagerRetained material_memory;
        std::unordered_map<std::shared_ptr<Material>, ManagedMemoryIndex> material_indices;

        std::vector<std::shared_ptr<Texture>> textures;
        std::list<std::pair<uint, std::vector<uint>>> added_textures; // the vector of frames the texture descriptor has been updated
        std::unordered_map<std::string, uint> texture_indices; // Texture path to texture index

        vk::Sampler default_sampler;

        uint frame_overlap;
        size_t texture_capacity;
        size_t initial_material_capacity;

        std::vector<vk::DescriptorSet> descriptor_sets;

        vma::Allocator* allocator;
        vk_util::UploadContext ctx;
    };

}

#endif