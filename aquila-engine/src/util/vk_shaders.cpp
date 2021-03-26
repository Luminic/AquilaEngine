#include "util/vk_shaders.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <optional>

namespace aq {

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