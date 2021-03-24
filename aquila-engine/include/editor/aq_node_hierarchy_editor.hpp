#include <memory>

#include <glm/glm.hpp>

#include <imgui.h>

#include "scene/aq_node.hpp"

namespace aq {

    void draw_tree(std::shared_ptr<Node> root_node, ImGuiTreeNodeFlags flags={});
    void draw_branch(std::shared_ptr<Node> current_node, glm::mat4 parent_transform=glm::mat4(1.0f), ImGuiTreeNodeFlags flags={});
    void draw_leaf(std::shared_ptr<Node> node, glm::mat4 hierarchical_transform, ImGuiTreeNodeFlags flags={});
    void draw_leaf(std::shared_ptr<Mesh> mesh, ImGuiTreeNodeFlags flags={});

}
