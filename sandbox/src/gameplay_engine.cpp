#include "gameplay_engine.hpp"

#include <iostream>

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
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    if (paused) unpause();
                    else pause();
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

    // aquila_engine.root_node->add_mesh(triangle_mesh);
    for (auto& n : aquila_engine.root_node) {
        n->add_mesh(triangle_mesh);
    }

    aquila_engine.upload_meshes();
}