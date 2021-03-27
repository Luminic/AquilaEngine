#ifndef EDITOR_AQUILA_NODE_HIERARCHY_EDITOR_HPP
#define EDITOR_AQUILA_NODE_HIERARCHY_EDITOR_HPP

#include <memory>
#include <unordered_set>

#include <glm/glm.hpp>

#include <imgui.h>

#include "scene/aq_material.hpp"
#include "scene/aq_mesh.hpp"
#include "scene/aq_node.hpp"

namespace aq {

    // Forward declare
    class InitializationEngine;

    class NodeHierarchyEditor {
    public:
        // If `RenderEngine` is set, `NodeHierarchyEditor` will be able to load models
        NodeHierarchyEditor(MaterialManager* material_manager, InitializationEngine* init_engine=nullptr);
        ~NodeHierarchyEditor();

        void draw_tree(std::shared_ptr<Node> root_node, ImGuiTreeNodeFlags flags=ImGuiTreeNodeFlags_AllowItemOverlap);
        void draw_branch(std::shared_ptr<Node> node, NodeHierarchyTraceback traceback, glm::mat4 parent_transform=glm::mat4(1.0f), ImGuiTreeNodeFlags flags={});
        void draw_branch(std::shared_ptr<Mesh> mesh, NodeHierarchyTraceback traceback, ImGuiTreeNodeFlags flags={});
        void draw_branch(std::shared_ptr<Material> material, ImGuiTreeNodeFlags flags={});
        void draw_leaf(std::shared_ptr<Node> node, glm::mat4 hierarchical_transform, ImGuiTreeNodeFlags flags={});
        void draw_leaf(std::shared_ptr<Mesh> mesh);
        void draw_leaf(std::shared_ptr<Material> material);

    private:
        // For when materials are updated
        MaterialManager* material_manager;
        InitializationEngine* init_engine;

        // Persistent data
        class NodeHierarchyEditorData* data;
    };


}

#endif