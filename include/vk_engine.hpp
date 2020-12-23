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
	uint64_t frame_number{0};

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

    vk::RenderPass render_pass;
    uint32_t image_count{0};
    std::vector<vk::Image> swap_chain_images;
	std::vector<vk::ImageView> swap_chain_image_views;
    std::vector<vk::Framebuffer> framebuffers;

    vk::CommandPool command_pool;
    vk::CommandBuffer main_command_buffer;

    vk::Fence render_fence;
    vk::Semaphore present_semaphore, render_semaphore;

    vk::PipelineLayout triangle_pipeline_layout;
    vk::Pipeline triangle_pipeline;

    bool init_vulkan();

    bool init_swapchain();
    bool init_default_renderpass();
    bool init_framebuffers();
    bool init_commands();
    bool init_sync_structures();

    bool init_pipelines();

    void draw();
};

#endif
