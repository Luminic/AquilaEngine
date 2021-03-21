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

    inline glm::vec3 ai_to_glm(const aiColor3D& from) {
        return glm::vec3(from.r, from.g, from.b);
    }

    inline glm::quat ai_to_glm(const aiQuaternion& from) {
        return glm::quat(from.w, from.x, from.y, from.z);
    }

    ModelLoader::ModelLoader(std::string directory, std::string file) : directory(directory), file(file) {
        std::string path = directory + file;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_RemoveRedundantMaterials);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Failed to load model: " << importer.GetErrorString() << std::endl;
            return;
        }

        aq_materials.resize(scene->mNumMaterials);

        root_node = process_node(scene->mRootNode, scene);
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

    // Gets the texture path from `ai_material` with the type `ai_tex_type` and creates a new
    // texture in `aq_material` with that path
    // Returns false if a texture of type `ai_tex_type` doesn't exist in `ai_material`
    bool transfer_corresponding_texture(
        aiTextureType ai_tex_type, Material::TextureType aq_tex_type, 
        aiMaterial* ai_material, std::shared_ptr<Material> aq_material,
        std::string directory, std::string file,
        const struct aiScene* ai_scene
    ) {
        if (ai_material->GetTextureCount(ai_tex_type) >= 1) {
            aiString ai_tex_path;
            ai_material->GetTexture(ai_tex_type, 0, &ai_tex_path);
            const aiTexture* tex = ai_scene->GetEmbeddedTexture(ai_tex_path.C_Str());
            if (tex) { // embedded texture
                std::string embedded_texture_path = directory + file + "::embedded::" + ai_tex_path.C_Str();

                aq_material->textures[aq_tex_type] = std::make_shared<Texture>();
                aq_material->textures[aq_tex_type]->upload_later(embedded_texture_path, (unsigned char*)tex->pcData, tex->mWidth, tex->mHeight, tex->achFormatHint);

            } else { // texture in file
                aq_material->textures[aq_tex_type] = std::make_shared<Texture>();
                aq_material->textures[aq_tex_type]->upload_later(directory + ai_tex_path.C_Str());
            }
            return true;
        }
        return false;
    }

    std::shared_ptr<Mesh> ModelLoader::process_mesh(struct aiMesh* ai_mesh, const struct aiScene* ai_scene) {
        std::shared_ptr<Mesh> aq_mesh = std::make_shared<Mesh>();

        // Load vertices
        aq_mesh->vertices.reserve(ai_mesh->mNumVertices);
        for (uint i=0; i<ai_mesh->mNumVertices; ++i) {
            glm::vec2 tex_coords(0.0f);
            if (ai_mesh->mTextureCoords[0])
                tex_coords = glm::vec2(ai_to_glm(ai_mesh->mTextureCoords[0][i]));
            aq_mesh->vertices.emplace_back(
                glm::vec4(ai_to_glm(ai_mesh->mVertices[i]), 1.0f),
                glm::vec4(ai_to_glm(ai_mesh->mNormals[i]), 0.0f),
                tex_coords
            );
        }

        // Load indices
        aq_mesh->indices.reserve(ai_mesh->mNumFaces * 3);
        for (uint i=0; i<ai_mesh->mNumFaces; ++i) {
            aiFace ai_face = ai_mesh->mFaces[i];
            assert(ai_face.mNumIndices == 3); // Only triangle primitives are supported
            for (uint j=0; j<3; ++j) {
                aq_mesh->indices.push_back(ai_face.mIndices[j]);
            }
        }

        // Load material
        if (aq_materials[ai_mesh->mMaterialIndex]) {
            aq_mesh->material = aq_materials[ai_mesh->mMaterialIndex];
        } else {
            aq_mesh->material = std::make_shared<Material>();
            aq_materials[ai_mesh->mMaterialIndex] = aq_mesh->material;

            aiMaterial* ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
            aiColor3D color;
            ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            aq_mesh->material->properties.albedo = glm::vec4(ai_to_glm(color), 0.0f);

            if (!transfer_corresponding_texture(aiTextureType_BASE_COLOR, Material::Albedo, ai_material, aq_mesh->material, directory, file, ai_scene))
                transfer_corresponding_texture(aiTextureType_DIFFUSE, Material::Albedo,     ai_material, aq_mesh->material, directory, file, ai_scene);

            if (!transfer_corresponding_texture(aiTextureType_DIFFUSE_ROUGHNESS, Material::Roughness, ai_material, aq_mesh->material, directory, file, ai_scene))
                transfer_corresponding_texture(aiTextureType_SHININESS, Material::Roughness,          ai_material, aq_mesh->material, directory, file, ai_scene);

            if (!transfer_corresponding_texture(aiTextureType_METALNESS, Material::Metalness, ai_material, aq_mesh->material, directory, file, ai_scene))
                transfer_corresponding_texture(aiTextureType_SPECULAR, Material::Metalness,   ai_material, aq_mesh->material, directory, file, ai_scene);

            if (!transfer_corresponding_texture(aiTextureType_AMBIENT_OCCLUSION, Material::AmbientOcclusion, ai_material, aq_mesh->material, directory, file, ai_scene))
                transfer_corresponding_texture(aiTextureType_AMBIENT, Material::AmbientOcclusion,            ai_material, aq_mesh->material, directory, file, ai_scene);
            
            if (!transfer_corresponding_texture(aiTextureType_NORMAL_CAMERA, Material::Normal, ai_material, aq_mesh->material, directory, file, ai_scene))
                transfer_corresponding_texture(aiTextureType_NORMALS, Material::Normal,        ai_material, aq_mesh->material, directory, file, ai_scene);
        }

        return aq_mesh;
    }

}