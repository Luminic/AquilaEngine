#include "aquila_engine.hpp"

#include <iostream>

namespace aq {

    AquilaEngine::AquilaEngine() {
        if (render_engine.init() != aq::RenderEngine::InitializationState::Initialized)
		    std::cerr << "Failed to initialize render engine." << std::endl;
    }

    AquilaEngine::~AquilaEngine() {
        render_engine.cleanup();
    }

    void AquilaEngine::update() {
        render_engine.update();
    }

    void AquilaEngine::draw(AbstractCamera* camera) {
        render_engine.draw(camera);
    }

}