#include "aquila_engine.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_sdl.h>

#include "scene/aq_texture.hpp"

#include "editor/aq_node_hierarchy_editor.hpp"

namespace aq {

    AquilaEngine::AquilaEngine() : render_engine(100), root_node(std::make_shared<Node>("Aquila Root")) {
        if (render_engine.init() != aq::RenderEngine::InitializationState::Initialized)
		    std::cerr << "Failed to initialize render engine." << std::endl;

        nhe = new NodeHierarchyEditor(&render_engine.material_manager, &render_engine.light_memory_manager, &render_engine);
    }

    AquilaEngine::~AquilaEngine() {
        delete nhe;

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

        ImGui::Begin("Hierarchy", nullptr, {});

        nhe->draw_tree(root_node);

        ImGui::End();
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
            render_engine.material_manager.add_material(material);
        }
    }

    bool AquilaEngine::process_sdl_event(const SDL_Event& sdl_event) {
        ImGui_ImplSDL2_ProcessEvent(&sdl_event);

        ImGuiIO io = ImGui::GetIO();
        
        switch (sdl_event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        case SDL_TEXTEDITING:
        case SDL_TEXTINPUT:
            if (io.WantCaptureKeyboard) return true;
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEWHEEL:
            if (io.WantCaptureMouse) return true;
            break;
        default:
            break;
        } 
        return false;
    }

}