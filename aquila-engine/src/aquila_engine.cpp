#include "aquila_engine.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

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
                mesh->upload(&render_engine.allocator);
            }
        }
    }

}