#include "gameplay_engine.hpp"

#include <aq_model_loader.hpp>
#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <chrono>

GameplayEngine::GameplayEngine() : camera_controller(&camera) {
    glm::ivec2 size = aquila_engine.get_render_window_size();
    camera.render_window_size_changed(size.x, size.y);
    camera.position.z = -2.0f;

    init_meshes();
}

GameplayEngine::~GameplayEngine() {}

void GameplayEngine::pause() {
    if (paused) return;
    SDL_SetRelativeMouseMode(SDL_FALSE);
    paused = true;
}

void GameplayEngine::unpause() {
    if (!paused) return;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    paused = false;
}

void GameplayEngine::run() {
    bool quit = false;

    auto begin_time = std::chrono::high_resolution_clock::now();

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_RESIZED:{
                    glm::ivec2 size = aquila_engine.get_render_window_size();
                    camera.render_window_size_changed(size.x, size.y);
                    break;}
                default:
                    break;
                }
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    if (paused) unpause();
                    else pause();
                    break;
                case SDLK_f: {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    double seconds_elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end_time - begin_time).count();
                    double fps = aquila_engine.get_frame_number() / seconds_elapsed;
                    std::cout << "average fps: " << fps << '\n';
                    break;}
                case SDLK_r:
                    begin_time = std::chrono::high_resolution_clock::now();
                    break;
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION: {
                if (!paused || (event.motion.state & SDL_BUTTON_MMASK))
                    camera_controller.rotate(event.motion.xrel, event.motion.yrel); 
                break;}
            default:
                break;
            }
        }

        CameraMovementFlags camera_movement(CameraMovementFlags::NONE);

        const uint8_t* keyboard_state = SDL_GetKeyboardState(nullptr);
        if (keyboard_state[SDL_SCANCODE_UP] || keyboard_state[SDL_SCANCODE_W])
            camera_movement |= CameraMovementFlags::FORWARD;
        if (keyboard_state[SDL_SCANCODE_DOWN] || keyboard_state[SDL_SCANCODE_S])
            camera_movement |= CameraMovementFlags::BACKWARD;
        if (keyboard_state[SDL_SCANCODE_LEFT] || keyboard_state[SDL_SCANCODE_A])
            camera_movement |= CameraMovementFlags::LEFTWARD;
        if (keyboard_state[SDL_SCANCODE_RIGHT] || keyboard_state[SDL_SCANCODE_D])
            camera_movement |= CameraMovementFlags::RIGHTWARD;
        if (keyboard_state[SDL_SCANCODE_LSHIFT])
            camera_movement |= CameraMovementFlags::DOWNWARD;
        if (keyboard_state[SDL_SCANCODE_SPACE])
            camera_movement |= CameraMovementFlags::UPWARD;
        
        camera_controller.move(camera_movement);

        camera.update();
        aquila_engine.update();
        aquila_engine.draw(&camera);
    }
}

void GameplayEngine::init_meshes() {
    std::shared_ptr<aq::Mesh> triangle_mesh = std::make_shared<aq::Mesh>();

    triangle_mesh->vertices.resize(4);

    triangle_mesh->vertices[0].position = {-1.0f,-1.0f, 0.0f, 1.0f};
    triangle_mesh->vertices[1].position = { 1.0f,-1.0f, 0.0f, 1.0f};
    triangle_mesh->vertices[2].position = { 1.0f, 1.0f, 0.0f, 1.0f};
    triangle_mesh->vertices[3].position = {-1.0f, 1.0f, 0.0f, 1.0f};

    triangle_mesh->vertices[0].color = {1.0f, 1.0f, 0.0f, 0.0f};
    triangle_mesh->vertices[1].color = {1.0f, 0.0f, 1.0f, 0.0f};
    triangle_mesh->vertices[2].color = {0.0f, 0.0f, 1.0f, 0.0f};
    triangle_mesh->vertices[3].color = {1.0f, 1.0f, 1.0f, 0.0f};

    triangle_mesh->indices = {
        0, 1, 3,
        3, 1, 2
    };

    glm::quat small_y_rot = glm::quat(glm::vec3(0.0f, -3.1415f / 4, 0.0f));
    auto prev = aquila_engine.root_node;
    for (uint i=0; i<30; ++i) {
        std::shared_ptr<aq::Node> child = std::make_shared<aq::Node>(glm::vec4(4.0f,-0.8f,0.0f,1.0f), small_y_rot);
        prev->add_node(child);
        prev = child;
    }

    aquila_engine.root_node->rotation = glm::quat(glm::vec3(0.0f, 3.1415f, 0.0f));


    aq::ModelLoader model_loader((std::string(SANDBOX_PROJECT_PATH) + "/resources/monkey.obj").c_str());

    std::vector<std::shared_ptr<aq::Node>> nodes;
    for (auto& n : aquila_engine.root_node) {
        nodes.push_back(n);
    }
    for (auto& node : nodes) {
        node->add_node(model_loader.get_root_node());
    }

    aquila_engine.upload_meshes();
}