#ifndef AQUILA_INITIALIZATION_ENGINE_HPP
#define AQUILA_INITIALIZATION_ENGINE_HPP

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

#include "vk_types.hpp"
#include "vk_initializers.hpp"

#include <unordered_map>
#include <deque>
#include <functional>

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

        // Returns `true` if initialization succeeds and `false` otherwise
        InitializationState init();
        bool wait_idle();
        void cleanup();

        InitializationState get_initialization_state();

    protected:
        InitializationState initialization_state{ InitializationState::Uninitialized };

        // Updated in `init_swapchain`; should always be correct
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
        std::unordered_map<std::string, bool> device_extensions;
        
        vk_init::GPUProperties gpu_properties;

        // Just an alias for `gpu_properties.graphics_present_queue_family`
        uint32_t& graphics_queue_family{ gpu_properties.graphics_present_queue_family };
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
        std::vector<vk::Image> swap_chain_images;
        std::vector<vk::ImageView> swap_chain_image_views;
        std::vector<vk::Framebuffer> framebuffers;

        // Initialized in multiple functions

        struct FrameObjects {
            vk::CommandPool command_pool;
            vk::CommandBuffer main_command_buffer;

            // Initialized in `init_sync_structures`

            vk::Fence render_fence;
            vk::Semaphore present_semaphore, render_semaphore;
        };
        static constexpr uint FRAME_OVERLAP = 3;
        std::array<FrameObjects, FRAME_OVERLAP> frame_objects{};
        FrameObjects& get_frame_objects(uint64_t frame_number) {return frame_objects[frame_number%FRAME_OVERLAP];}

        bool init_vulkan_resources();
        void cleanup_vulkan_resources();
        bool init_swapchain_resources();
        void cleanup_swapchain_resources();
        virtual bool init_render_resources();
        virtual void cleanup_render_resources();

        virtual bool resize_window();

        bool init_command_pool();
        bool choose_surface_format();
        bool init_default_renderpass();

        bool init_command_buffers();
        bool init_swapchain();
        bool init_framebuffers();
        bool init_sync_structures();
    
    private:
        DeletionQueue deletion_queue;
        DeletionQueue swap_chain_deletion_queue;
    };

}

#endif
