#include "editor/aq_node_hierarchy_editor.hpp"

#include <cfloat>

#include <glm/gtc/type_ptr.hpp>

#include <misc/cpp/imgui_stdlib.h>

namespace aq {

    class NodeHierarchyEditorData {};

    NodeHierarchyEditor::NodeHierarchyEditor(MaterialManager* material_manager) : material_manager(material_manager) {
        data = new NodeHierarchyEditorData();
    };

    NodeHierarchyEditor::~NodeHierarchyEditor() {
        delete data;
    };

    void NodeHierarchyEditor::draw_tree(std::shared_ptr<Node> root_node, ImGuiTreeNodeFlags flags) {
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        draw_branch(root_node, glm::mat4(1.0f), flags);
    }

    void NodeHierarchyEditor::draw_branch(std::shared_ptr<Node> node, glm::mat4 parent_transform, ImGuiTreeNodeFlags flags) {
        if (ImGui::TreeNodeEx(node.get(), flags, "Node %s", node->name.c_str())) {
            glm::mat4 hierarchical_transform = parent_transform * node->get_model_matrix();

            if (ImGui::TreeNodeEx("Properties", flags)) {
                draw_leaf(node, hierarchical_transform, flags);
                ImGui::TreePop();
            }

            for(auto& child_mesh : node->get_child_meshes()) {
                draw_branch(child_mesh, flags);
            }

            for (auto& child_node : node->get_child_nodes()) {
                draw_branch(child_node, hierarchical_transform, flags);
            }

            ImGui::TreePop();
        }
    }

    void NodeHierarchyEditor::draw_branch(std::shared_ptr<Mesh> mesh, ImGuiTreeNodeFlags flags) {
        if (ImGui::TreeNodeEx(mesh.get(), flags, "Mesh %s", mesh->name.c_str())) {
            draw_leaf(mesh);
            if (mesh->material) draw_branch(mesh->material, flags);
            ImGui::TreePop();
        }
    }

    void NodeHierarchyEditor::draw_branch(std::shared_ptr<Material> material, ImGuiTreeNodeFlags flags) {
        if (ImGui::TreeNodeEx(material.get(), flags, "Material %s", material->name.c_str())) {
            draw_leaf(material);
            ImGui::TreePop();
        }
    }

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Node> node, glm::mat4 hierarchical_transform, ImGuiTreeNodeFlags flags) {
        ImGui::Text("Memory location: %p", (void*)node.get());
        ImGui::InputText("Name", &node->name);
        ImGui::Spacing();

        ImGui::Text("Transform:");
        ImGui::DragFloat3("Position", glm::value_ptr(node->position), 0.01f);
        ImGui::DragFloat4("Rotation", glm::value_ptr(node->rotation), 0.01f);
        ImGui::DragFloat3("Scale", glm::value_ptr(node->scale), 0.01f);
        ImGui::Spacing();

        ImGui::Text("Original Transform:");
        ImGui::DragFloat4("##otr0", glm::value_ptr(node->org_transform[0]), 0.01f);
        ImGui::DragFloat4("##otr1", glm::value_ptr(node->org_transform[1]), 0.01f);
        ImGui::DragFloat4("##otr2", glm::value_ptr(node->org_transform[2]), 0.01f);
        ImGui::DragFloat4("##otr3", glm::value_ptr(node->org_transform[3]), 0.01f);
        ImGui::Spacing();

        ImGui::Text("Hierarchical Transform:");
        ImGui::DragFloat4("##htr0", glm::value_ptr(hierarchical_transform[0]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
        ImGui::DragFloat4("##htr1", glm::value_ptr(hierarchical_transform[1]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
        ImGui::DragFloat4("##htr2", glm::value_ptr(hierarchical_transform[2]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
        ImGui::DragFloat4("##htr3", glm::value_ptr(hierarchical_transform[3]), 0.01f, 0.0f, 0.0f, "%.3f", ImGuiSliderFlags_NoInput);
    }

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Mesh> mesh) {
        ImGui::Text("Memory location: %p", (void*)mesh.get());
        ImGui::InputText("Name", &mesh->name);
        ImGui::Spacing();
    }

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Material> material) {
        ImGui::Text("Memory location: %p, Buffer index: %u", (void*)material.get(), material_manager->get_material_index(material));
        ImGui::InputText("Name", &material->name);
        ImGui::Spacing();

        // Update material info on the gpu?
        bool update_material_info = false;

        ImGui::Text("Texture indices:");
        ImGui::Indent();
            ImGui::Text("Albedo: %p",           material->textures[Material::Albedo          ].get());
            ImGui::Text("Roughness: %p",        material->textures[Material::Roughness       ].get());
            ImGui::Text("Metalness: %p",        material->textures[Material::Metalness       ].get());
            ImGui::Text("AmbientOcclusion: %p", material->textures[Material::AmbientOcclusion].get());
            ImGui::Text("Normal: %p",           material->textures[Material::Normal          ].get());
        ImGui::Unindent();
        ImGui::Spacing();

        ImGui::Text("Material info:");
        if (ImGui::DragFloat3("Albedo",    glm::value_ptr(material->properties.albedo),  0.01f, 0.0f, FLT_MAX)) update_material_info = true;
        if (ImGui::DragFloat3("Ambient",   glm::value_ptr(material->properties.ambient), 0.01f, 0.0f, FLT_MAX)) update_material_info = true;
        if (ImGui::DragFloat ("Roughness", &material->properties.roughness,              0.01f, 0.0f, 1.0f   )) update_material_info = true;
        if (ImGui::DragFloat ("Metalness", &material->properties.metalness,              0.01f, 0.0f, 1.0f   )) update_material_info = true;

        if (update_material_info)
            material_manager->update_material(material);
    }

}