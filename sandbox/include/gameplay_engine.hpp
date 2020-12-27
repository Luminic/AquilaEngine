#ifndef GAMEPLAY_ENGINE_HPP
#define GAMEPLAY_ENGINE_HPP

#include <aquila_engine.hpp>
#include "camera_controller.hpp"

class GameplayEngine {
public:
    GameplayEngine();
    ~GameplayEngine();

    void pause();
    void unpause();

    void run();

protected:
    aq::AquilaEngine aquila_engine;
    aq::DefaultCamera camera;
    CameraController camera_controller;

    bool paused = true;
};

#endif