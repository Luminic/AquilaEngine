#include "editor/aq_node_hierarchy_editor.hpp"

#include <cfloat>
#include <variant>

#include <glm/gtc/type_ptr.hpp>

#include <misc/cpp/imgui_stdlib.h>

#include "init_engine.hpp"
#include "scene/aq_model_loader.hpp"
#include "editor/aq_mesh_creator.hpp"
#include "editor/file_browser.hpp"

// Helper to display a little (?) mark which shows a tooltip when hovered.
// Copied from imgui_demo.cpp
static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

namespace aq {

    struct NodeHierarchyEditorData {
        bool keep_transform = false;
        std::string error_text;

        int chosen_light_type = (int)Light::Type::Point; // Have to use int so it can be used in combo box
        int chosen_mesh_type = 0;
        int sphere_creation_res[2] = {32, 16};
        FileBrowser* file_browser = nullptr;

        // Variables for Euler Angle Rotation Editor
        glm::quat original_rotation;
        glm::vec3 yaw_pitch_roll_editor;

        // Cleanup just in case
        ~NodeHierarchyEditorData() {
            delete file_browser;
        }
    };

    struct NHEDragDropPayload {
        enum class Type {
            Node,
            Mesh
        };
        Type type;
        std::shared_ptr<Node> parent_node;
        glm::mat4 parental_transform;
        std::variant<std::shared_ptr<Node>, std::shared_ptr<Mesh>> data;
    };

    NodeHierarchyEditor::NodeHierarchyEditor(MaterialManager* material_manager, LightMemoryManager* light_memory_manager, InitializationEngine* init_engine) : material_manager(material_manager), light_memory_manager(light_memory_manager), init_engine(init_engine) {
        data = new NodeHierarchyEditorData();
    };

    NodeHierarchyEditor::~NodeHierarchyEditor() {
        delete data;
    };

    void NodeHierarchyEditor::draw_tree(std::shared_ptr<Node> root_node, ImGuiTreeNodeFlags flags) {
        draw_branch(root_node, {}, glm::mat4(1.0f), flags);


        ImGui::Separator();
        ImGui::Checkbox("Keep Transform", &data->keep_transform);
        ImGui::SameLine();
        HelpMarker("Nodes will have the same transformation when moved to different hierarchies. Might have side effects on other instances of the moved node.");


        ImGui::Separator();
        if (ImGui::CollapsingHeader("Create New...")) {
            if (ImGui::Button("Create New Mesh")) {
                if (init_engine) ImGui::OpenPopup("Mesh Creator");
                else data->error_text = "NHE cannot create meshes with null `InitEngine`";
            }
            if (ImGui::BeginPopup("Mesh Creator")) {
                ImGui::Combo("Type", &data->chosen_mesh_type, "Cube\0Sphere\0");

                switch (data->chosen_mesh_type) {
                case 0: break;
                case 1:
                    ImGui::DragInt2("Resolution", data->sphere_creation_res, 1.0f, 3, INT_MAX, "%d", ImGuiSliderFlags_AlwaysClamp);
                    break;
                default: break;
                }

                if (ImGui::Button("Create")) {
                    std::shared_ptr<Mesh> mesh;
                    switch (data->chosen_mesh_type) {
                        case 0: mesh = mesh_creator::create_cube(); break;
                        case 1: mesh = mesh_creator::create_sphere(data->sphere_creation_res[0], data->sphere_creation_res[1]); break;
                        default: data->error_text = "Mesh shape unimplemented"; break;
                    }
                    if (mesh) {
                        mesh->upload(init_engine->get_allocator(), init_engine->get_default_upload_context());
                        root_node->add_mesh(mesh);
                    }
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            if (ImGui::Button("Create New Material")) { data->error_text = "Material viewer currently unimplemented."; }
            if (ImGui::Button("Create New Node")) { root_node->add_node(std::make_shared<Node>("New Node")); }
            if (ImGui::Button("Create New Light")) {
                if (light_memory_manager) ImGui::OpenPopup("Light Creator");
                else data->error_text = "NHE cannot create lights with null `LightMemoryManager`";
            }
            if (ImGui::BeginPopup("Light Creator")) {
                ImGui::Combo("Type", &data->chosen_light_type, Light::TypeNames.data(), Light::TypeNames.size());
                if (ImGui::Button("Create")) {
                    std::shared_ptr<Light> new_light;
                    switch ((Light::Type) data->chosen_light_type) {
                        case Light::Type::Point:
                            new_light = std::make_shared<PointLight>("New Light");
                            break;
                        case Light::Type::Sun:
                        case Light::Type::Area:
                        case Light::Type::Spot:
                            data->error_text = "Light type currently unimplemented";
                            break;
                        default:
                            data->error_text = "Could not create light: unknown class";
                            break;
                    }
                    if (new_light) {
                        new_light->set_memory_manager(light_memory_manager);
                        root_node->add_node(new_light);
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel")) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }

            if (ImGui::Button("Load Model") && !data->file_browser) {
                if (init_engine) data->file_browser = new FileBrowser();
                else data->error_text = "NHE cannot load models with null `InitEngine`";
            }
            if (data->file_browser) {
                switch (data->file_browser->get_status()) {
                case FileBrowser::Status::Selecting:
                    data->file_browser->draw();
                    break;
                case FileBrowser::Status::Confirmed: {
                    ModelLoader loader(data->file_browser->current_directory(), data->file_browser->current_selection());
                    // Upload Meshes
                    for (auto& node : loader.get_root_node())
                        for (auto& mesh : node->get_child_meshes())
                            if (!mesh->is_uploaded()) // A Mesh might appear several times in a node tree so make sure it's only uploaded once
                                mesh->upload(init_engine->get_allocator(), init_engine->get_default_upload_context());
                    // Upload Materials
                    for (auto& material : loader.get_materials())
                        material_manager->add_material(material);
                    // Add root model node to hierarchy
                    root_node->add_node(loader.get_root_node());

                } // Intentional fallthrough to `FileBrowser::Status::Canceled`
                case FileBrowser::Status::Canceled:
                    delete data->file_browser;
                    data->file_browser = nullptr;
                    break;
                }
            }

        }

        if (!data->error_text.empty()) {
            ImGui::Separator();
            ImGui::TextColored({1.0f,0.0f,0.0f,1.0f}, "Error: %s", data->error_text.c_str());
        }
    }

    void NodeHierarchyEditor::draw_branch(std::shared_ptr<Node> node, NodeHierarchyTraceback traceback, glm::mat4 parent_transform, ImGuiTreeNodeFlags flags) {
        if (!node) return;
        glm::mat4 hierarchical_transform = parent_transform * node->get_model_matrix();

        bool node_open = ImGui::TreeNodeEx(node.get(), flags, "Node %s", node->name.c_str());

        // Enable drag & drop (root node cannot be moved)
        if (traceback.node && ImGui::BeginDragDropSource()) {
            NHEDragDropPayload payload_data{ NHEDragDropPayload::Type::Node, traceback.node, parent_transform, node };
            ImGui::SetDragDropPayload("DND_NHE_Rearrange_Branches", &payload_data, sizeof(NHEDragDropPayload));
            ImGui::Text("Move Node %s", node->name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_NHE_Rearrange_Branches")) {
                assert(payload->DataSize == sizeof(NHEDragDropPayload));
                NHEDragDropPayload* payload_data = (NHEDragDropPayload*)payload->Data;
                
                switch (payload_data->type) {
                case NHEDragDropPayload::Type::Node: {
                    std::shared_ptr<Node> moved_node = std::get<std::shared_ptr<Node>>(payload_data->data);

                    // Make sure no loops are created
                    bool loop = false;
                    // Iterate over all of the moved node's children (at every depth) including the moved node
                    for (auto& moved_child_node : moved_node) {
                        // Iterate over the current node's parents
                        NodeHierarchyTraceback node_hierarchy_traceback{node, &traceback};
                        for (NodeHierarchyTraceback* current = &node_hierarchy_traceback; current != nullptr; current = current->prev) {
                            if (current->node == moved_child_node) {
                                data->error_text = "Drop would create a node loop; aborting.";
                                loop = true;
                                break;
                            }
                        }
                        if (loop) break;
                    }

                    if (!loop) {
                        payload_data->parent_node->remove_node(moved_node.get());
                        node->add_node(moved_node);
                        if (data->keep_transform) {
                            if (glm::determinant(hierarchical_transform) >= 0.001f) {
                                glm::mat4 inv_parent_transform = glm::inverse(hierarchical_transform);
                                moved_node->org_transform = inv_parent_transform * payload_data->parental_transform * moved_node->org_transform;
                            } else {
                                data->error_text = "Could not keep transform (un-invertable parent transform).";
                            }
                        }
                    }
                } break;
                case NHEDragDropPayload::Type::Mesh: {
                    std::shared_ptr<Mesh> moved_mesh = std::get<std::shared_ptr<Mesh>>(payload_data->data);
                    payload_data->parent_node->remove_mesh(moved_mesh.get());
                    node->add_mesh(moved_mesh);
                } break;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (node_open) {
            // Delete? (Root node cannot be deleted)
            if (traceback.node) {
                ImGui::SameLine();
                bool delete_node = ImGui::SmallButton("x");
                if (delete_node) {
                    traceback.node->remove_node(node.get());
                    // Don't draw anything else
                    ImGui::TreePop();
                    return;
                }
            }

            if (ImGui::TreeNodeEx("Properties (Node)", flags)) {
                draw_leaf(node, hierarchical_transform, flags);
                ImGui::TreePop();
            }
            if (std::shared_ptr<Light> light = std::dynamic_pointer_cast<Light>(node)) {
                if (ImGui::TreeNodeEx("Properties (Light)", flags)) {
                    draw_leaf(light, parent_transform, flags);
                    ImGui::TreePop();
                }
            }

            for(auto& child_mesh : node->get_child_meshes()) {
                draw_branch(child_mesh, {node, &traceback}, flags);
            }

            for (auto& child_node : node->get_child_nodes()) {
                draw_branch(child_node, {node, &traceback}, hierarchical_transform, flags);
            }

            ImGui::TreePop();
        }
    }

    void NodeHierarchyEditor::draw_branch(std::shared_ptr<Mesh> mesh, NodeHierarchyTraceback traceback, ImGuiTreeNodeFlags flags) {
        if (!mesh) return;

        bool node_open = ImGui::TreeNodeEx(mesh.get(), flags, "Mesh %s", mesh->name.c_str());

        // Enable drag & drop (can be drag source but not drop destnation)
        if (ImGui::BeginDragDropSource()) {
            NHEDragDropPayload payload_data{ NHEDragDropPayload::Type::Mesh, traceback.node, {}, mesh };
            ImGui::SetDragDropPayload("DND_NHE_Rearrange_Branches", &payload_data, sizeof(NHEDragDropPayload));
            ImGui::Text("Move Mesh %s", mesh->name.c_str());
            ImGui::EndDragDropSource();
        }

        if (node_open) {
            ImGui::SameLine();
            bool delete_mesh = ImGui::SmallButton("x");
            if (delete_mesh) {
                traceback.node->remove_mesh(mesh.get());
                // Don't draw anything else
                ImGui::TreePop();
                return;
            }

            draw_leaf(mesh);
            if (mesh->material) draw_branch(mesh->material, flags);
            ImGui::TreePop();
        }
    }

    void NodeHierarchyEditor::draw_branch(std::shared_ptr<Material> material, ImGuiTreeNodeFlags flags) {
        if (!material) return;

        if (ImGui::TreeNodeEx(material.get(), flags, "Material %s", material->name.c_str())) {
            draw_leaf(material);
            ImGui::TreePop();
        }
    }

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Node> node, glm::mat4 hierarchical_transform, ImGuiTreeNodeFlags flags) {
        if (!node) return;

        ImGui::Text("Memory location: %p, References: %u", (void*)node.get(), node.use_count());
        ImGui::InputText("Name", &node->name);
        ImGui::Spacing();

        ImGui::Text("Transform:");
        ImGui::DragFloat3("Position", glm::value_ptr(node->position), 0.01f);
        ImGui::DragFloat4("Rotation", glm::value_ptr(node->rotation), 0.01f);
        ImGui::DragFloat3("Scale", glm::value_ptr(node->scale), 0.01f);

        if (ImGui::Button("Edit Rotation w/ Euler Angles")) {
            ImGui::OpenPopup("EulerAngleEditor");
            data->original_rotation = node->rotation;
            data->yaw_pitch_roll_editor = glm::vec3(0.0f);
        }
        if (ImGui::BeginPopup("EulerAngleEditor")) {
            ImGui::DragFloat("Yaw", &data->yaw_pitch_roll_editor[0], 0.1, -360, 360);
            ImGui::DragFloat("Pitch", &data->yaw_pitch_roll_editor[1], 0.1, -360, 360);
            ImGui::DragFloat("Roll", &data->yaw_pitch_roll_editor[2], 0.1, -360, 360);

            glm::quat qyaw = glm::angleAxis(data->yaw_pitch_roll_editor[0], glm::vec3(0.0f,1.0f,0.0f));
            glm::quat qpitch = glm::angleAxis(data->yaw_pitch_roll_editor[1], glm::vec3(1.0f,0.0f,0.0f));
            glm::quat qroll = glm::angleAxis(data->yaw_pitch_roll_editor[2], glm::vec3(0.0f,0.0f,1.0f));
            node->rotation = qyaw * qpitch * qroll * data->original_rotation;

            ImGui::EndPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Apply Transform")) {
            node->org_transform = node->get_model_matrix();
            node->position = glm::vec3(0.0f);
            node->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            node->scale = glm::vec3(1.0f);
        }

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

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Light> light, glm::mat4 parent_transform, ImGuiTreeNodeFlags flags) {
        if (!light) return;

        Light::Properties light_properties = light->get_properties(parent_transform);

        switch (light->get_type()) {
        case Light::Type::Point: {
            std::shared_ptr<PointLight> pl = std::dynamic_pointer_cast<PointLight>(light);
            assert(pl);
            ImGui::Text("PointLight");
            ImGui::Text("Shadow map texture index: %u", light_properties.shadow_map_ti);
            ImGui::ColorEdit3("Color", glm::value_ptr(pl->color));
            ImGui::DragFloat("Power", &pl->color.w, 0.01f, 0.0f, FLT_MAX);
            ImGui::DragFloat3("Final Position", glm::value_ptr(light_properties.position), 0.01f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::SameLine(); HelpMarker("Final position of the light. Modify light position (relative to parent node) in node properties.");
        } break;
        case Light::Type::Sun: {}; // Unimplemented so fallthrough to default
        case Light::Type::Area: {}; // Unimplemented so fallthrough to default
        case Light::Type::Spot: {}; // Unimplemented so fallthrough to default
        default: {
            ImGui::Text("Unknown Light Type: %u", (uint)light_properties.type);
            ImGui::Text("Shadow map texture index: %u", light_properties.shadow_map_ti);
            ImGui::ColorEdit3("Color", glm::value_ptr(light_properties.color), ImGuiColorEditFlags_NoPicker);
            ImGui::DragFloat("Power", &light_properties.color.w, 0.01f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::DragFloat3("Final Position", glm::value_ptr(light_properties.position), 0.01f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::SameLine(); HelpMarker("Final position of the light. Modify light position (relative to parent node) in node properties.");
            ImGui::DragFloat3("Direction", glm::value_ptr(light_properties.direction), 0.01f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoInput);
            ImGui::DragFloat4("Misc", glm::value_ptr(light_properties.misc), 0.01f, 0.0f, FLT_MAX, "%.3f", ImGuiSliderFlags_NoInput);
        } break;
        }
    }

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Mesh> mesh) {
        if (!mesh) return;

        ImGui::Text("Memory location: %p, References: %u", (void*)mesh.get(), mesh.use_count());
        ImGui::InputText("Name", &mesh->name);
        ImGui::Spacing();
    }

    void NodeHierarchyEditor::draw_leaf(std::shared_ptr<Material> material) {
        if (!material) return;

        ImGui::Text("Memory location: %p, References: %u, Buffer index: %u", (void*)material.get(), material.use_count(), material_manager->get_material_index(material));
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