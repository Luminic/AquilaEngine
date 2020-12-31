#ifndef SCENE_AQUILA_MODEL_LOADER_HPP
#define SCENE_AQUILA_MODEL_LOADER_HPP

#include "aq_mesh.hpp"
#include "aq_node.hpp"

#include <memory>

struct aiNode;
struct aiMesh;
struct aiScene;

namespace aq {

    class ModelLoader {
    public:
        ModelLoader(const char* path);

        std::shared_ptr<Node> get_root_node();
    
    private:
        std::shared_ptr<Node> process_node(aiNode* ai_node, const aiScene* ai_scene);
        std::shared_ptr<Mesh> process_mesh(aiMesh* ai_mesh, const aiScene* ai_scene);

        std::shared_ptr<Node> root_node=nullptr;
    };

}

#endif