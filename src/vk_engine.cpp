#include "vk_engine.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>

VulkanEngine::VulkanEngine() {}

VulkanEngine::~VulkanEngine() {
    cleanup(); // Just in case
}


bool VulkanEngine::init() {
    std::cout << "init\n";
    initialized = true;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_extent.width,
        window_extent.height,
        window_flags
    );

    if (!window) {
        std::cerr << "SDL window failed to initialze: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!init_vulkan()) return false;

    return true;
}

void VulkanEngine::run() {
    std::cout << "run\n";

    SDL_Event event;
	bool quit = false;

	while (!quit) {
		while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    std::cout << event.key.keysym.sym << "\n";
                    break;
                case SDL_KEYUP:
                    std::cout << event.key.keysym.sym << "\n";
                    break;
                default:
                    break;
            }
		}

		draw();
	}
}

void VulkanEngine::cleanup() {
    if (initialized) {
        std::cout << "cleanup\n";

        if (main_command_buffer) device.freeCommandBuffers(command_pool, main_command_buffer);
        main_command_buffer = nullptr;

        if (command_pool) device.destroyCommandPool(command_pool);
        command_pool = nullptr;

        if (swap_chain) device.destroySwapchainKHR(swap_chain);
        swap_chain = nullptr;

        if (surface) instance.destroy(surface);
        surface = nullptr;

        if (device) device.destroy();
        device = nullptr;

        if (instance) instance.destroy();
        instance = nullptr;

        if (window) SDL_DestroyWindow(window);
        window = nullptr;

        initialized = false;
    }
}

bool VulkanEngine::init_vulkan() {
    device_extensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;

    if (!create_instance()) return false;

    VkSurfaceKHR sdl_surface;
    SDL_Vulkan_CreateSurface(window, instance, &sdl_surface);
    surface = static_cast<vk::SurfaceKHR>(sdl_surface);

    if (!select_physical_device()) return false;
    if (!create_logical_device()) return false;
    if (!init_swapchain()) return false;
    if (!init_commands()) return false;

    return true;
}

bool VulkanEngine::create_instance() {
    vk::ApplicationInfo app_info("Vulkan Test", 1, "Vulkan Engine", 1, VK_API_VERSION_1_2);
    vk::InstanceCreateInfo instance_create_info({}, &app_info);

    // Query needed extensions

    unsigned int sdl_extension_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr)) return false;
    std::vector<const char*> sdl_extensions(sdl_extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions.data())) return false;

    for (auto extension : sdl_extensions) {
        extensions[std::string(extension)] = true;
    }

    // Check extension availability

    auto [eiep_result, supported_extensions] = vk::enumerateInstanceExtensionProperties();

    std::vector<const char*> requested_extensions;

    if (eiep_result == vk::Result::eSuccess) {
        bool has_all_required;
        std::tie(has_all_required, requested_extensions) = vk_init::get_requested_and_supported_extensions(extensions, supported_extensions, true, "Unsupported extension");
        if (!has_all_required) return false;
    } else {
        std::cerr << "Unable to query supported extensions: " << int(eiep_result) << std::endl;
        // Try requesting all extensions anyways--they might be supported
        for (auto extension : extensions) {
            extension.second = true;
            requested_extensions.push_back(extension.first.c_str());
        }
    }

    instance_create_info.setPEnabledExtensionNames(requested_extensions);
    

    #ifndef NDEBUG // Add Vulkan Layers for debug builds

        std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

        // Check layer availability

        auto [eilp_result, supported_layers] = vk::enumerateInstanceLayerProperties();

        if (eilp_result == vk::Result::eSuccess) {
            for (int i=validation_layers.size()-1; i >= 0; --i) {
                const char* requested_layer = validation_layers[i];
                bool supported = false;
                for (auto supported_layer : supported_layers) {
                    if (strcmp(requested_layer, supported_layer.layerName) == 0) {
                        supported = true;
                        break;
                    }
                }
                if (!supported) {
                    std::cerr << "Unsupported layer: " << requested_layer << std::endl;
                    validation_layers.erase(validation_layers.begin() + i);
                }
            }
            instance_create_info.setPEnabledLayerNames(validation_layers);
        } else {
            std::cerr << "Unable to query suported layers: " << int(eilp_result) << ". No layers enabled." << std::endl;
        }

    #endif

    // Create Instance

    vk::Result ci_result;
    std::tie(ci_result, instance) = vk::createInstance(instance_create_info);
    CHECK_VK_RESULT_RF(ci_result, "Failed to create vulkan instance");

    return true;
}

bool VulkanEngine::select_physical_device() {
    auto [epd_result, physical_devices] = instance.enumeratePhysicalDevices();
    CHECK_VK_RESULT_RF(epd_result, "Failed to query GPUs");

    if (physical_devices.size() == 0) {
        std::cerr << "Failed to find GPU." << std::endl;
        return false;
    }
    
    int best_score = -1;
    for (auto& physical_device : physical_devices) {
        vk_init::GPUProperties tmp(physical_device, device_extensions, surface);
        if (tmp.score > best_score) {
            chosen_gpu = physical_device;
            gpu_properties = tmp;
            best_score = tmp.score;
        }
    }
    if (best_score < 0) {
        chosen_gpu = nullptr;
        std::cerr << "No suitable GPU." << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanEngine::create_logical_device() {
    float queue_priorites = 1.0f;
    std::array<vk::DeviceQueueCreateInfo, 1> device_queue_create_infos{{
        {{}, graphics_queue_family, 1, &queue_priorites}
    }};

    // Warn about unsupported device extensions and update `device_extensions`
    // so that supported extensions are `true` and unsupported extensions are
    // `false`
    for (auto& extension : device_extensions) {
        extension.second = false;
        for (auto rs_extension : gpu_properties.supported_requested_device_extensions) {
            if (extension.first == rs_extension) {
                extension.second = true;
            }
        }
        if (!extension.second) {
            std::cerr << "Unsupported device extension: " << extension.first << std::endl;
        }
    }

    vk::PhysicalDeviceFeatures device_features({});

    vk::DeviceCreateInfo device_create_info({}, device_queue_create_infos, {}, gpu_properties.supported_requested_device_extensions, &device_features);

    vk::Result cd_result;
    std::tie(cd_result, device) = chosen_gpu.createDevice(device_create_info);
    CHECK_VK_RESULT_RF(cd_result, "Failed to create logical device");

    graphics_queue = device.getQueue(graphics_queue_family, 0);

    return true;
}

bool VulkanEngine::init_swapchain() {
    vk_init::SwapChainSupportDetails& sw_ch_support = gpu_properties.sw_ch_support;

    // Should be checked for in `rate_gpu` and ineligible GPUs should not
    // be considered but just in case something goes wrong
    if (sw_ch_support.formats.size() == 0 ||
        sw_ch_support.present_modes.size() == 0) {
        std::cerr << "Device missing swap chain support: No supported format/present mode found." << std::endl;
        return false;
    }

    // Get surface format
    surface_format = sw_ch_support.formats[0];
    for (auto& available_format : sw_ch_support.formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
            available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            surface_format = available_format;
        }
    }

    // Get present mode
    present_mode = vk::PresentModeKHR::eFifo; // Always supported
    for (auto& available_present_mode : sw_ch_support.present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            present_mode = available_present_mode;
        }
    }

    // Get extent
    if (sw_ch_support.capabilities.currentExtent.width != UINT32_MAX) {
        window_extent = sw_ch_support.capabilities.currentExtent;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        window_extent = vk::Extent2D(
            std::clamp(uint32_t(width), sw_ch_support.capabilities.minImageExtent.width, sw_ch_support.capabilities.maxImageExtent.width),
            std::clamp(uint32_t(height), sw_ch_support.capabilities.minImageExtent.height, sw_ch_support.capabilities.maxImageExtent.height)
        );
    }

    // Figure out number of swapchain images
    uint32_t min_image_count = 3;
    if (sw_ch_support.capabilities.maxImageCount == 0) {
        min_image_count = std::max(min_image_count, sw_ch_support.capabilities.minImageCount);
    } else {
        min_image_count = std::clamp(min_image_count, sw_ch_support.capabilities.minImageCount, sw_ch_support.capabilities.maxImageCount);
    }

    // Actually create swapchain
    vk::SwapchainCreateInfoKHR swap_chain_create_info(
        {}, // flags
        surface,
        min_image_count,
        surface_format.format,
        surface_format.colorSpace,
        window_extent,
        1, // image array layers
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        {}, // queue family array
        sw_ch_support.capabilities.currentTransform, // pre-transform
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        present_mode,
        VK_TRUE, // clipped
        {} // old swapchain
    );

    vk::Result csc_result;
    std::tie(csc_result, swap_chain) = device.createSwapchainKHR(swap_chain_create_info);
    CHECK_VK_RESULT_RF(csc_result, "Failed to create swap chain");

    return true;
}

bool VulkanEngine::init_commands() {
    vk::CommandPoolCreateInfo command_pool_create_info(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        graphics_queue_family
    );

    vk::Result ccp_result;
    std::tie(ccp_result, command_pool) = device.createCommandPool(command_pool_create_info);
    CHECK_VK_RESULT_RF(ccp_result, "Failed to create command pool");

    vk::CommandBufferAllocateInfo cmd_buff_alloc_info(command_pool, vk::CommandBufferLevel::ePrimary, 1);

    auto[acb_result, cmd_buffs] = device.allocateCommandBuffers(cmd_buff_alloc_info);
    CHECK_VK_RESULT_RF(acb_result, "Failed to allocate command buffer(s)");

    main_command_buffer = cmd_buffs[0];

    return true;
}


void VulkanEngine::draw() {

}