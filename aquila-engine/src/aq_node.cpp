#include "aq_node.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace aq {

    Node::Node(
        glm::vec3 position, 
        glm::quat rotation, 
        glm::vec3 scale
    ) : position(position), rotation(rotation), scale(scale) {}

    Node::~Node() {}

    void Node::add_node(std::shared_ptr<Node> node) {
        child_nodes.push_back(node);
    }

    void Node::remove_node(Node* node) {
        for (auto it=child_nodes.begin(); it != child_nodes.end(); ++it) {
            if (it->get() == node) {
                child_nodes.erase(it);
                break;
            }
        }
    }

    void Node::add_mesh(std::shared_ptr<Mesh> mesh) {
        child_meshes.push_back(mesh);
    }

    glm::mat4 Node::get_model_matrix() {
        glm::mat4 model(1.0f);
        model = glm::scale(model, scale);
        model = glm::mat4_cast(rotation) * model;
        model = glm::translate(model, position);
        return model;
    }

}