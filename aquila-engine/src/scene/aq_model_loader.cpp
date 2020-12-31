#include "scene/aq_model_loader.hpp"

#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace aq {

    inline glm::mat4 ai_to_glm(const aiMatrix4x4& from) {
        glm::mat4 to(
            {{from.a1,from.b1,from.c1,from.d1}
            ,{from.a2,from.b2,from.c2,from.d2}
            ,{from.a3,from.b3,from.c3,from.d3}
            ,{from.a4,from.b4,from.c4,from.d4}}
        );
        return to;
    }

    inline glm::vec3 ai_to_glm(const aiVector3D& from) {
        return glm::vec3(from.x, from.y, from.z);
    }

    inline glm::quat ai_to_glm(const aiQuaternion& from) {
        return glm::quat(from.w, from.x, from.y, from.z);
    }

    ModelLoader::ModelLoader(const char* path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_RemoveRedundantMaterials);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Failed to load model: " << importer.GetErrorString() << std::endl;
            return;
        }

        root_node = process_node(scene->mRootNode, scene);
    }

    std::shared_ptr<Node> ModelLoader::get_root_node() {
        return root_node;
    }

    std::shared_ptr<Node> ModelLoader::process_node(aiNode* ai_node, const aiScene* ai_scene) {
        std::shared_ptr<Node> aq_node = std::make_shared<Node>();

        aq_node->org_transform = ai_to_glm(ai_node->mTransformation);

        for (uint i=0; i<ai_node->mNumMeshes; ++i) {
            aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
            aq_node->add_mesh(process_mesh(ai_mesh, ai_scene));
        }

        for (uint i=0; i<ai_node->mNumChildren; ++i) {
            aq_node->add_node(process_node(ai_node->mChildren[i], ai_scene));
        }

        return aq_node;
    }

    std::shared_ptr<Mesh> ModelLoader::process_mesh(struct aiMesh* ai_mesh, const struct aiScene* ai_scene) {
        std::shared_ptr<Mesh> aq_mesh = std::make_shared<Mesh>();

        aq_mesh->vertices.reserve(ai_mesh->mNumVertices);
        for (uint i=0; i<ai_mesh->mNumVertices; ++i) {
            aq_mesh->vertices.emplace_back(
                glm::vec4(ai_to_glm(ai_mesh->mVertices[i]), 1.0f),
                glm::vec4(ai_to_glm(ai_mesh->mNormals[i]), 0.0f),
                glm::vec4(ai_to_glm(ai_mesh->mNormals[i]), 0.0f) // Make the color the normal for now
            );
        }

        aq_mesh->indices.reserve(ai_mesh->mNumFaces * 3);
        for (uint i=0; i<ai_mesh->mNumFaces; ++i) {
            aiFace ai_face = ai_mesh->mFaces[i];
            assert(ai_face.mNumIndices == 3); // Only triangle primitives are supported
            for (uint j=0; j<3; ++j) {
                aq_mesh->indices.push_back(ai_face.mIndices[j]);
            }
        }

        return aq_mesh;
    }

}