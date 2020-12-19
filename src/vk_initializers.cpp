#include "vk_initializers.hpp"

#include <iostream>

namespace vk_init {

    std::pair<bool, std::vector<const char*>> get_requested_and_supported_extensions(
        std::unordered_map<std::string, bool>& requested,
        const std::vector<vk::ExtensionProperties>& supported,
        bool modify,
        std::optional<std::string> error_msg) 
    {
        bool has_all_required_extensions = true;
        std::vector<const char*> requested_and_supported_extensions;

        for (auto& extension : requested) {
            bool required = extension.second;
            if (modify) extension.second = false;
            for (auto& supported_extension : supported) {
                if (extension.first == supported_extension.extensionName) {
                    if (modify) extension.second = true;
                    requested_and_supported_extensions.push_back(extension.first.c_str());
                    break;
                }
            }
            if (!extension.second && error_msg) {
                if (required) std::cerr << "Critical deficiency: ";
                std::cerr << error_msg.value() << ": " << extension.first << std::endl;
                has_all_required_extensions = false;
            }
        }
        return {has_all_required_extensions, requested_and_supported_extensions};
    }

    SwapChainSupportDetails::SwapChainSupportDetails(
        vk::SurfaceCapabilitiesKHR capabilities,
        std::vector<vk::SurfaceFormatKHR> formats,
        std::vector<vk::PresentModeKHR> present_modes
    ) : capabilities(capabilities),
        formats(formats),
        present_modes(present_modes) {}
    
    SwapChainSupportDetails::SwapChainSupportDetails(vk::PhysicalDevice gpu, vk::SurfaceKHR surface) {
        vk::Result gsc_result;
        std::tie(gsc_result, capabilities) = gpu.getSurfaceCapabilitiesKHR(surface);
        CHECK_VK_RESULT(gsc_result, "Failed to query GPU surface capabilities");
        
        vk::Result gsfs_result;
        std::tie(gsfs_result, formats) = gpu.getSurfaceFormatsKHR(surface);
        CHECK_VK_RESULT(gsfs_result, "Failed to query GPU surface formats");

        vk::Result gspms_result;
        std::tie(gspms_result, present_modes) = gpu.getSurfacePresentModesKHR(surface);
        CHECK_VK_RESULT(gspms_result, "Failed to query GPU surface present modes");
    }

    GPUProperties::GPUProperties(
        int score,
        std::vector<vk::ExtensionProperties> supported_device_extensions,
        std::vector<const char*> supported_requested_device_extensions,
        std::vector<vk::QueueFamilyProperties> supported_queue_families,
        uint32_t graphics_present_queue_family,
        SwapChainSupportDetails sw_ch_support
    ) : score(score),
        supported_device_extensions(supported_device_extensions),
        supported_requested_device_extensions(supported_requested_device_extensions),
        supported_queue_families(supported_queue_families),
        graphics_present_queue_family(graphics_present_queue_family),
        sw_ch_support(sw_ch_support) {}
    
    GPUProperties::GPUProperties(
        vk::PhysicalDevice gpu,
        std::unordered_map<std::string, bool>& requested_device_extensions,
        vk::SurfaceKHR surface
    ) {
        score = 1;

        // Check queue support
        bool has_graphics_present_support = false;
        supported_queue_families = gpu.getQueueFamilyProperties();
        for (size_t i=0; i<supported_queue_families.size(); ++i) {
            auto [gss_res, surface_support] = gpu.getSurfaceSupportKHR(i, surface);
            if (gss_res != vk::Result::eSuccess) surface_support = false;
            if ((supported_queue_families[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                surface_support
            ) {
                graphics_present_queue_family = i;
                has_graphics_present_support = true;
                break;
            }
        }
        if (!has_graphics_present_support) {
            graphics_present_queue_family = UINT32_MAX;
            score = -1;
            return;
        }

        // Check extension support
        vk::Result edep_result;
        std::tie(edep_result, supported_device_extensions) = gpu.enumerateDeviceExtensionProperties();
        if (edep_result == vk::Result::eSuccess) {
            bool has_required_extensions;
            std::tie(has_required_extensions, supported_requested_device_extensions) = get_requested_and_supported_extensions(
                requested_device_extensions,
                supported_device_extensions,
                false
            );
            if (has_required_extensions) {
                score += 1000 * supported_requested_device_extensions.size();
            } else {
                score = -1;
                return;
            }
        } else {
            score = -1;
            return;
        }

        // Check swap chain support
        sw_ch_support = SwapChainSupportDetails(gpu, surface);

        if (sw_ch_support.formats.size() == 0 ||
            sw_ch_support.present_modes.size() == 0) {
            score = -1;
            return;
        }

        score += sw_ch_support.formats.size() * 100;
        score += sw_ch_support.present_modes.size() * 100;
    }

}
