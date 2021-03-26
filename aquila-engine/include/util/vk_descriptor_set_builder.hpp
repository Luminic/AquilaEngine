#ifndef UTIL_AQUILA_DESCRIPTOR_SET_BUILDER_HPP
#define UTIL_AQUILA_DESCRIPTOR_SET_BUILDER_HPP

#include <vector>

#include "util/vk_types.hpp"

namespace aq {

    class DescriptorSetAllocator {
    public:
        DescriptorSetAllocator();

        void init(vk::Device device);
        void destroy();

        vk::DescriptorPool new_pool(const std::vector<vk::DescriptorPoolSize>& pool_sizes);

    private:
        std::vector<vk::DescriptorPool> descriptor_pools;
        vk::Device device;
    };

    class DescriptorSetBuilder {
    public:
        DescriptorSetBuilder(DescriptorSetAllocator* descriptor_set_allocator, vk::Device device, uint multiplicity=1);

        DescriptorSetBuilder& add_binding(vk::DescriptorSetLayoutBinding binding);
        std::vector<vk::DescriptorSet> build();
        vk::DescriptorSetLayout get_layout() { return layout; }; // Only valid after `build()`
    
    private:
        DescriptorSetAllocator* descriptor_set_allocator;
        uint multiplicity;

        std::vector<vk::DescriptorPoolSize> pool_sizes;
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        vk::DescriptorSetLayout layout = nullptr;

        vk::Device device;
    };

}

#endif