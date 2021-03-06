#ifndef SCENE_AQUILA_MESH_HPP
#define SCENE_AQUILA_MESH_HPP

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "scene/aq_vertex.hpp"
#include "scene/aq_material.hpp"

namespace aq {

    class Mesh {
    public:
        Mesh();
        Mesh(const std::string& name);
        Mesh(const std::vector<Vertex>& vertices, const std::vector<Index>& indices);
        Mesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<Index>& indices);
        ~Mesh();

        std::vector<Vertex> vertices;
        std::vector<Index> indices;

        std::shared_ptr<Material> material;

        // `name` should never be used as an ID; it's just an easy way for users to identify meshes
        std::string name;

        // Stores both the index data and the vertex data in the same buffer
        // The index data is first and the vertex data starts at `vertex_data_offset`
        AllocatedBuffer combined_iv_buffer;
        vk::DeviceSize vertex_data_offset;

        // Uploads the mesh on CPU visible memory
        void upload(vma::Allocator* allocator);
        // Uploads the mesh on GPU local memory (not necessarily CPU visible)
        void upload(vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        // Will only free `vertex_buffer` if it was allocated through `upload`
        void free(); 

        bool is_uploaded() {return bool(combined_iv_buffer.buffer);}

    private:
        AllocatedBuffer create_buffer_with_iv_data(vk::BufferUsageFlags buffer_usage, vma::MemoryUsage memory_usage);

        vma::Allocator* allocator = nullptr;
    };

}

#endif