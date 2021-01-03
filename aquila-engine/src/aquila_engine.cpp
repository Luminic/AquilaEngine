#include "aquila_engine.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "scene/aq_texture.hpp"

namespace aq {

    AquilaEngine::AquilaEngine() : root_node(std::make_shared<Node>()) {
        if (render_engine.init() != aq::RenderEngine::InitializationState::Initialized)
		    std::cerr << "Failed to initialize render engine." << std::endl;
        
    }

    AquilaEngine::~AquilaEngine() {
        render_engine.wait_idle();

        for (auto& node : root_node) {
            for (auto& mesh : node->get_child_meshes()) {
                mesh->free();
            }
        }
        render_engine.cleanup();
    }

    void AquilaEngine::update() {
        render_engine.update();
    }

    void AquilaEngine::draw(AbstractCamera* camera) {
        render_engine.draw(camera, root_node);
    }

    void AquilaEngine::upload_meshes() {
        for (auto& node : root_node) {
            for (auto& mesh : node->get_child_meshes()) {
                if (!mesh->is_uploaded()) // A Mesh might appear several times in a node tree so make sure it's only uploaded once
                    mesh->upload(&render_engine.allocator, render_engine.get_default_upload_context());
            }
        }
    }

    void AquilaEngine::upload_materials(const std::vector<std::shared_ptr<Material>>& materials) {
        for (auto& material : materials) {
            if (!material->is_managed())
                render_engine.material_manager.add_material(material);
        }
    }

}