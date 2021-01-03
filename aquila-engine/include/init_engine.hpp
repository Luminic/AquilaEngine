#ifndef AQUILA_INITIALIZATION_ENGINE_HPP
#define AQUILA_INITIALIZATION_ENGINE_HPP

#include <vector>
#include <unordered_map>
#include <deque>
#include <array>
#include <functional>

#include <SDL2/SDL.h>

#include <glm/glm.hpp>

#include "util/vk_types.hpp"
#include "util/vk_initializers.hpp"

namespace aq {

    class DeletionQueue {
    public:
        void push_function(std::function<void()>&& function) {
            deletors.push_back(function);
        }
        void flush() {
            for (auto it = deletors.rbegin(); it != deletors.rend(); ++it) {
                (*it)(); // Call functors
            }
            deletors.clear();
        }
    private:
        std::deque<std::function<void()>> deletors;
    };

    class InitializationEngine {
    public:
        InitializationEngine();
        virtual ~InitializationEngine();

        enum class InitializationState {
            Uninitialized,
            FailedVulkanObjectsInitialization,
            VulkanObjectsInitialized,
            FailedSwapChainInitialization,
            Initialized, // Swapchain initialized
            FailedInitialization // Misc/Unknown failure state
        };

        InitializationState init();
        bool wait_idle();
        void cleanup();

        InitializationState get_initialization_state();
        SDL_Window* get_window() { return window; }

    protected:
        InitializationState initialization_state{ InitializationState::Uninitialized };

        // Updated in `init_swapchain`

        // Should usually be correct but if the window was just resized, might be one frame behind
        vk::Extent2D window_extent{ 1700, 900 };

        // Initialized in `init`

        SDL_Window* window{ nullptr };

        // Initialized in `init_vulkan_resources`

        vk::Instance instance;
        vk::SurfaceKHR surface;
        vk::PhysicalDevice chosen_gpu;
        vk::Device device;

        // After `select_physical_device`, the `bool` represents if the extension is enabled
        // Before, the `bool` replresents if the extension is required
        // Not all possible extensions will be in `extensions`; only the requested ones
        std::unordered_map<std::string, bool> extensions;
        std::unordered_map<std::string, bool> device_extensions; // Same behavior as `extensions`
        
        vk_init::GPUSupport gpu_support;
        vk::PhysicalDeviceProperties gpu_properties;
        vk::PhysicalDeviceFeatures gpu_features;

        // Just an alias for `gpu_support.graphics_present_queue_family`
        uint32_t& graphics_queue_family{ gpu_support.graphics_present_queue_family };
        vk::Queue graphics_queue;

        vma::Allocator allocator;

        // Initialized in `choose_surface_format`

        vk::SurfaceFormatKHR surface_format;
        vk::PresentModeKHR present_mode;

        // Initialized in `init_default_renderpass`

        vk::RenderPass render_pass;
        vk::Format depth_format;

        // Initialized in `init_swapchain`

        vk::SwapchainKHR swap_chain;
        vk::ImageView depth_image_view;
        AllocatedImage depth_image;

        // Initialized in `init_framebuffers`

        uint32_t image_count{0};
        std::vector<vk::Image> swap_chain_images; // Should be sized `image_count` after initialization
        std::vector<vk::ImageView> swap_chain_image_views; // Should be sized `image_count` after initialization
        std::vector<vk::Framebuffer> framebuffers; // Should be sized `image_count` after initialization

        // Initialized in multiple functions

        struct FrameObjects {
            vk::CommandPool command_pool; // Initialized in `init_command_pools`
            vk::CommandBuffer main_command_buffer; // Initialized in `init_command_buffers`

            // Initialized in `init_swap_chain_sync_structures`

            vk::Fence render_fence;
            vk::Semaphore present_semaphore, render_semaphore;
        };
        static constexpr uint FRAME_OVERLAP = 3;
        std::array<FrameObjects, FRAME_OVERLAP> frame_objects{};
        FrameObjects& get_frame_objects(uint64_t frame_number) {return frame_objects[frame_number%FRAME_OVERLAP];}

        vk::CommandPool upload_command_pool;
        vk::Fence upload_fence; // Initialized in `init_vulkan_resources` (needed for `init_render_resources`)

        // Usually used for `immediate_submit`
        vk_util::UploadContext get_default_upload_context(); 

        bool init_vulkan_resources();
        void cleanup_vulkan_resources();
        bool init_swapchain_resources();
        void cleanup_swapchain_resources();
        virtual bool init_render_resources();
        virtual void cleanup_render_resources();

        virtual bool resize_window();

        bool init_command_pools();
        bool choose_surface_format();
        bool init_default_renderpass();

        bool init_command_buffers();
        bool init_swapchain();
        bool init_framebuffers();
        bool init_swap_chain_sync_structures();
    
    private:
        DeletionQueue deletion_queue;
        DeletionQueue swap_chain_deletion_queue;
    };

}

#endif
