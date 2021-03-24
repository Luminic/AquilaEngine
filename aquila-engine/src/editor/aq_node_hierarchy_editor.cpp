#include "editor/aq_node_hierarchy_editor.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace aq {

    void draw_tree(std::shared_ptr<Node> root_node, ImGuiTreeNodeFlags flags) {
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        draw_branch(root_node, glm::mat4(1.0f), flags);
    }

    void draw_branch(std::shared_ptr<Node> current_node, glm::mat4 parent_transform, ImGuiTreeNodeFlags flags) {
        if (ImGui::TreeNodeEx(current_node.get(), flags, "Node")) {
            glm::mat4 hierarchical_transform = parent_transform * current_node->get_model_matrix();

            draw_leaf(current_node, hierarchical_transform, flags);

            for(auto& child_mesh : current_node->get_child_meshes()) {
                draw_leaf(child_mesh, flags);
            }

            for (auto& child_node : current_node->get_child_nodes()) {
                draw_branch(child_node, hierarchical_transform, flags);
            }

            ImGui::TreePop();
        }
    }

    void draw_leaf(std::shared_ptr<Node> node, glm::mat4 hierarchical_transform, ImGuiTreeNodeFlags flags) {
        if (ImGui::TreeNodeEx("Properties", flags)) {
            ImGui::Text("Memory location: %p", (void*)node.get());
            ImGui::DragFloat3("Position", glm::value_ptr(node->position), 0.01f);
            ImGui::DragFloat4("Rotation", glm::value_ptr(node->rotation), 0.01f);
            ImGui::DragFloat3("Scale", glm::value_ptr(node->scale), 0.01f);
            ImGui::Text("Original Transform:");
            ImGui::DragFloat4("##otr0", glm::value_ptr(node->org_transform[0]), 0.01f);
            ImGui::DragFloat4("##otr1", glm::value_ptr(node->org_transform[1]), 0.01f);
            ImGui::DragFloat4("##otr2", glm::value_ptr(node->org_transform[2]), 0.01f);
            ImGui::DragFloat4("##otr3", glm::value_ptr(node->org_transform[3]), 0.01f);
            ImGui::Text("Hierarchical Transform:");
            ImGui::DragFloat4("##htr0", glm::value_ptr(hierarchical_transform[0]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::DragFloat4("##htr1", glm::value_ptr(hierarchical_transform[1]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::DragFloat4("##htr2", glm::value_ptr(hierarchical_transform[2]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::DragFloat4("##htr3", glm::value_ptr(hierarchical_transform[3]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
                
            ImGui::TreePop();
        }
    }

    void draw_leaf(std::shared_ptr<Mesh> mesh, ImGuiTreeNodeFlags flags) {
        if (ImGui::TreeNodeEx(mesh.get(), flags, "Mesh")) {
            ImGui::Text("Memory location: %p", (void*)mesh.get());
            ImGui::TreePop();
        }
    }

}