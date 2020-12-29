#include "aquila_engine.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace aq {

    AquilaEngine::AquilaEngine() : root_node(std::make_shared<Node>()) {
        if (render_engine.init() != aq::RenderEngine::InitializationState::Initialized)
		    std::cerr << "Failed to initialize render engine." << std::endl;

        
        // std::cout << "root " << root_node << '\n';
        auto child1 = std::make_shared<Node>();
        // std::cout << "child1 " << child1 << '\n';
        auto child1a = std::make_shared<Node>();
        // std::cout << "child1a " << child1a << '\n';
        auto child1b = std::make_shared<Node>();
        // std::cout << "child1b " << child1b << '\n';
        auto child1bx = std::make_shared<Node>();
        // std::cout << "child1bx " << child1bx << '\n';

        child1->add_node(child1a);
        child1->add_node(child1b);

        child1b->add_node(child1bx);

        auto child2 = std::make_shared<Node>();
        std::cout << "child2 " << child2 << '\n';

        auto child3 = std::make_shared<Node>();
        std::cout << "child3 " << child3 << '\n';

        root_node->add_node(child1);
        root_node->add_node(child2);
        root_node->add_node(child3);

        root_node->scale = glm::vec3(2.0f,1.0f,1.0f);
        child1b->rotation = glm::quat(glm::vec3(3.14f,0.0f,0.0f));
        child1bx->position = glm::vec3(0.0f,0.0f,1.0f);

        child2->position = glm::vec3(2.0f,0.0f,0.0f);
        child3->position = glm::vec3(-2.0f,0.0f,0.0f);
        child3->rotation = glm::quat(glm::vec3(0.0f,1.57f,0.0f));

        /*
        for (auto it=hbegin(root_node); it != hend(root_node); ++it) {
            std::cout << "node " << *it << "\n";
            glm::mat4 pm = it.get_transform();
            for (int i=0; i<4; ++i) {
                for (int j=0; j<4; ++j) {
                    std::cout << pm[i][j] << ", ";
                }
                std::cout << '\n';
            }
        }
        */
        
    }

    AquilaEngine::~AquilaEngine() {
        render_engine.wait_idle();

        for (auto& node : root_node) {
            for (auto& mesh : node->child_meshes) {
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
            for (auto& mesh : node->child_meshes) {
                mesh->upload(&render_engine.allocator);
            }
        }
    }

}