#include <render_engine.hpp>

int main(int argc, char* argv[]) {
	aq::RenderEngine engine;

	if (engine.init() != aq::RenderEngine::InitializationState::Initialized) {
		return 1;
	}
	
	engine.run();

	engine.cleanup();

	return 0;
}
