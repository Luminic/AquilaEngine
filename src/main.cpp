#include "vk_engine.hpp"

int main(int argc, char* argv[]) {
	VulkanEngine engine;

	if (!engine.init())
		return 1;
	
	engine.run();

	engine.cleanup();

	return 0;
}
