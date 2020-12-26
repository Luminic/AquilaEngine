#include "vk_render_engine.hpp"

int main(int argc, char* argv[]) {
	VulkanRenderEngine engine;

	std::cout << PROJECT_PATH << '\n';

	if (engine.init() != VulkanRenderEngine::InitializationState::Initialized) {
		return 1;
	}
	
	engine.run();

	engine.cleanup();

	return 0;
}
