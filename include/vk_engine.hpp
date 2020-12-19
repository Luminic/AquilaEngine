#ifndef VK_ENGINE_HPP
#define VK_ENGINE_HPP

#include "vk_types.hpp"
#include "vk_initializers.hpp"

#include <unordered_map>

class VulkanEngine {
public:
    VulkanEngine();
    ~VulkanEngine();

    // Returns `true` if initialization succeeds and `false` otherwise
    bool init();
    void run();
    void cleanup();

private:
    bool initialized{ false };
	int frame_number{0};

	vk::Extent2D window_extent{ 1700, 900 };

    struct SDL_Window* window{ nullptr };

    vk::Instance instance;
    // vk::DebugUtilsMessengerEXT debug_messenger;
    vk::SurfaceKHR surface;
    vk::PhysicalDevice chosen_gpu;
    vk::Device device;

    // After `select_physical_device`, the `bool` represents if the extension is enabled
    // Before, the `bool` replresents if the extension is required
    // Not all possible extensions will be in `extensions`; only the requested ones
    std::unordered_map<std::string, bool> extensions;
    std::unordered_map<std::string, bool> device_extensions;
    
    vk_init::GPUProperties gpu_properties;

    // This graphics queue family must also support present operations
    // `UINT32_MAX` is a "Does not exist" value
    // Just an alias for `gpu_properties.graphics_present_queue_family`
    uint32_t& graphics_queue_family{ gpu_properties.graphics_present_queue_family };
    vk::Queue graphics_queue;

    vk::SurfaceFormatKHR surface_format;
    vk::PresentModeKHR present_mode;
    vk::SwapchainKHR swap_chain;

    vk::CommandPool command_pool;
    vk::CommandBuffer main_command_buffer;

    bool init_vulkan();
    bool create_instance();
    bool select_physical_device();
    bool create_logical_device();
    bool init_swapchain();
    bool init_commands();

    void draw();
};

#endif
