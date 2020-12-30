#include "aq_node.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace aq {

    Node::Node(
        glm::vec3 position, 
        glm::quat rotation, 
        glm::vec3 scale,
        glm::mat4 org_transform
    ) : position(position), rotation(rotation), scale(scale), org_transform(org_transform) {}

    Node::~Node() {}

    void Node::add_node(std::shared_ptr<Node> node) {
        child_nodes.push_back(node);
    }

    void Node::remove_node(Node* node) {
        for (size_t i=child_nodes.size(); i>0; --i) {
            size_t current_index = i - 1;
            if (child_nodes[current_index].get() == node) {
                child_nodes.erase(child_nodes.begin() + current_index);
            }
        }
    }

    void Node::add_mesh(std::shared_ptr<Mesh> mesh) {
        child_meshes.push_back(mesh);
    }

    void Node::remove_mesh(Mesh* mesh) {
        for (size_t i=child_meshes.size(); i>0; --i) {
            size_t current_index = i - 1;
            if (child_meshes[current_index].get() == mesh) {
                child_meshes.erase(child_meshes.begin() + current_index);
            }
        }
    }

    glm::mat4 Node::get_model_matrix() {
        glm::mat4 model = org_transform;
        model = glm::scale(model, scale);
        model = glm::mat4_cast(rotation) * model;
        model = glm::translate(model, position);
        return model;
    }

}