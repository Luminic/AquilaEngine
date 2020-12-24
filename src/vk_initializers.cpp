#include "vk_initializers.hpp"

#include <iostream>
#include <fstream>

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
        update(gpu, surface);
    }

    bool SwapChainSupportDetails::update(vk::PhysicalDevice gpu, vk::SurfaceKHR surface) {
        vk::Result gsc_result;
        std::tie(gsc_result, capabilities) = gpu.getSurfaceCapabilitiesKHR(surface);
        CHECK_VK_RESULT_R(gsc_result, false, "Failed to query GPU surface capabilities");
        
        vk::Result gsfs_result;
        std::tie(gsfs_result, formats) = gpu.getSurfaceFormatsKHR(surface);
        CHECK_VK_RESULT_R(gsfs_result, false, "Failed to query GPU surface formats");

        vk::Result gspms_result;
        std::tie(gspms_result, present_modes) = gpu.getSurfacePresentModesKHR(surface);
        CHECK_VK_RESULT_R(gspms_result, false, "Failed to query GPU surface present modes");

        return true;
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


    VulkanInitializer::VulkanInitializer(vk::Instance& instance, vk::PhysicalDevice& gpu, vk::Device& device) :
        instance(instance), gpu(gpu), device(device) {}

    bool VulkanInitializer::create_instance(
        const char* app_name,
        uint32_t app_version,
        const char* engine_name,
        uint32_t engine_version,
        std::unordered_map<std::string, bool>& extensions
    ) {
        vk::ApplicationInfo app_info(app_name, app_version, engine_name, engine_version, VK_API_VERSION_1_2);
        vk::InstanceCreateInfo instance_create_info({}, &app_info);

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
        CHECK_VK_RESULT_R(ci_result, false, "Failed to create vulkan instance");

        return true;
    }

    bool VulkanInitializer::choose_gpu(
        std::unordered_map<std::string, bool>& device_extensions,
        vk::SurfaceKHR compatible_surface
    ) {
        auto [epd_result, physical_devices] = instance.enumeratePhysicalDevices();
        CHECK_VK_RESULT_R(epd_result, false, "Failed to query GPUs");

        if (physical_devices.size() == 0) {
            std::cerr << "Failed to find GPU." << std::endl;
            return false;
        }
        
        int best_score = -1;
        for (auto& physical_device : physical_devices) {
            vk_init::GPUProperties tmp(physical_device, device_extensions, compatible_surface);
            if (tmp.score > best_score) {
                gpu = physical_device;
                gpu_properties = tmp;
                best_score = tmp.score;
            }
        }
        if (best_score < 0) {
            gpu = nullptr;
            std::cerr << "No suitable GPU." << std::endl;
            return false;
        }
        
        return true;
    }

    bool VulkanInitializer::create_device(std::unordered_map<std::string, bool>& device_extensions) {
        float queue_priorites = 1.0f;
        std::array<vk::DeviceQueueCreateInfo, 1> device_queue_create_infos{{
            {{}, gpu_properties.graphics_present_queue_family, 1, &queue_priorites}
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

        vk::PhysicalDeviceFeatures device_features{};

        vk::DeviceCreateInfo device_create_info({}, device_queue_create_infos, {}, gpu_properties.supported_requested_device_extensions, &device_features);

        vk::Result cd_result;
        std::tie(cd_result, device) = gpu.createDevice(device_create_info);
        CHECK_VK_RESULT_R(cd_result, false, "Failed to create logical device");

        return true;
    }

    GPUProperties VulkanInitializer::get_gpu_properties() {
        return gpu_properties;
    }


    std::optional<std::vector<uint32_t>> read_binary_file(const char* file_path) {
        std::ifstream file(file_path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Failed to open shader file " << file_path << std::endl;
            return {};
        }

        size_t file_size = (size_t) file.tellg();
        std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

        file.seekg(0);
        file.read((char*)buffer.data(), file_size);
        file.close();

        return buffer;
    }

    vk::ShaderModule load_shader_module(const char* file_path, vk::Device device) {
        std::vector<uint32_t> buffer = read_binary_file(file_path).value_or(std::vector<uint32_t>{});
        if (buffer.empty()) {
            std::cerr << "Failed to read shader file " << file_path << std::endl;
            return nullptr;
        }

        vk::ShaderModuleCreateInfo shader_module_create_info({}, buffer);        

        auto [csm_result, shader_module] = device.createShaderModule(shader_module_create_info);
        CHECK_VK_RESULT_R(csm_result, nullptr, "Failed to load shader module");

        return shader_module;
    }

    vk::UniqueShaderModule load_shader_module_unique(const char* file_path, vk::Device device) {
        std::vector<uint32_t> buffer = read_binary_file(file_path).value_or(std::vector<uint32_t>{});
        if (buffer.empty()) {
            std::cerr << "Failed to read shader file " << file_path << std::endl;
            return vk::UniqueShaderModule(nullptr);
        }

        vk::ShaderModuleCreateInfo shader_module_create_info({}, buffer);
        
        auto [csm_result, shader_module] = device.createShaderModuleUnique(shader_module_create_info);
        CHECK_VK_RESULT_R(csm_result, vk::UniqueShaderModule(nullptr), "Failed to load shader module");

        return std::move(shader_module);
    }

}
