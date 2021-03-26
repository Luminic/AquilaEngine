#ifndef UTIL_AQUILA_SHADERS_HPP
#define UTIL_AQUILA_SHADERS_HPP

#include "util/vk_types.hpp"

namespace aq {

    // Buffer bindings for the per-frame `vk::DescriptorSet`s
    enum class PerFrameBufferBindings {
        MaterialPropertiesBuffer = 0,
        DefaultSampler = 1,
        Textures = 2
    };

    /*
    Ideal Buffer Bindings:
    enum class PerFrameBufferBindings {
        CameraBuffer = 0,
        MaterialPropertiesBuffer = 1,
        LightPropertiesBuffer = 2,
        LightShadowMaps = 3, ???
        LightShadowCubeMaps = 4 ???
    };
    enum class LongLivedBufferBindings {
        Sampler = 1,
        Textures = 2 ??? (might be the same as PerFrameBufferBindings::LightShadowMaps)
    };
    */

    vk::ShaderModule load_shader_module(const char* file_path, vk::Device device);
    vk::UniqueShaderModule load_shader_module_unique(const char* file_path, vk::Device device);

}

#endif