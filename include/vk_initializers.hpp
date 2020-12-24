#ifndef VK_INITIALIZERS_HPP
#define VK_INITIALIZERS_HPP

#include "vk_types.hpp"

namespace vk_init {
    // Returns every `const char*` in both `requested` and `supported`.
    // The ptr will point to the the keys in `requested`.
    // 
    // If `modify` is `true`, The values in `requested` will be set to
    // `true` or `false` depending on if it is or isn't in `supported`.
    std::pair<bool, std::vector<const char*>> get_requested_and_supported_extensions(
        std::unordered_map<std::string, bool>& requested,
        const std::vector<vk::ExtensionProperties>& supported,
        bool modify = true,
        std::optional<std::string> error_msg = {}
    );


    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> present_modes;

        SwapChainSupportDetails(
            vk::SurfaceCapabilitiesKHR capabilities = {},
            std::vector<vk::SurfaceFormatKHR> formats = {},
            std::vector<vk::PresentModeKHR> present_modes = {}
        );

        SwapChainSupportDetails(vk::PhysicalDevice gpu, vk::SurfaceKHR surface);

        bool update(vk::PhysicalDevice gpu, vk::SurfaceKHR surface);
    };


    struct GPUProperties {
        int score;

        std::vector<vk::ExtensionProperties> supported_device_extensions;
        std::vector<const char*> supported_requested_device_extensions;

        std::vector<vk::QueueFamilyProperties> supported_queue_families;
        uint32_t graphics_present_queue_family;

        SwapChainSupportDetails sw_ch_support;

        GPUProperties(
            int score = 0,
            std::vector<vk::ExtensionProperties> supported_device_extensions = {},
            std::vector<const char*> supported_requested_device_extensions = {},
            std::vector<vk::QueueFamilyProperties> supported_queue_families = {},
            uint32_t graphics_present_queue_family = UINT32_MAX,
            SwapChainSupportDetails sw_ch_support = {}
        );

        GPUProperties(
            vk::PhysicalDevice gpu,
            std::unordered_map<std::string, bool>& requested_device_extensions,
            vk::SurfaceKHR surface
        );
    };

    class VulkanInitializer {
    public:
        VulkanInitializer(
            vk::Instance& instance,
            vk::PhysicalDevice& gpu,
            vk::Device& device
        );

        bool create_instance(
            const char* app_name,    uint32_t app_version,
            const char* engine_name, uint32_t engine_version,
            std::unordered_map<std::string, bool>& extensions
        );

        bool choose_gpu(
            std::unordered_map<std::string, bool>& device_extensions,
            vk::SurfaceKHR compatible_surface
        );

        bool create_device(std::unordered_map<std::string, bool>& device_extensions);

        GPUProperties get_gpu_properties();

    private:
        vk::Instance& instance;
        vk::PhysicalDevice& gpu;
        vk::Device& device;
        GPUProperties gpu_properties;
    };


    vk::ShaderModule load_shader_module(const char* file_path, vk::Device device);
    vk::UniqueShaderModule load_shader_module_unique(const char* file_path, vk::Device device);
}

#endif