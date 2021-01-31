#ifndef SCENE_AQUILA_MODEL_LOADER_HPP
#define SCENE_AQUILA_MODEL_LOADER_HPP

#include <memory>
#include <vector>
#include <string>

#include "aq_texture.hpp"
#include "aq_material.hpp"
#include "aq_mesh.hpp"
#include "aq_node.hpp"

struct aiNode;
struct aiMesh;
struct aiScene;

namespace aq {

    class ModelLoader {
    public:
        // Loads model with name `file` located in `directory`
        // Finds textures relative to `directory`
        ModelLoader(std::string directory, std::string file);

        std::shared_ptr<Node> get_root_node() { return root_node; }
        const std::vector<std::shared_ptr<Material>>& get_materials() { return aq_materials; }

    private:
        std::shared_ptr<Node> process_node(aiNode* ai_node, const aiScene* ai_scene);
        std::shared_ptr<Mesh> process_mesh(aiMesh* ai_mesh, const aiScene* ai_scene);

        std::shared_ptr<Node> root_node = nullptr;

        std::vector<std::shared_ptr<Material>> aq_materials;

        std::string directory;
        std::string file;
    };

}

#endif