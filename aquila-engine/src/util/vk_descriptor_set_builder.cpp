#include "util/vk_descriptor_set_builder.hpp"

namespace aq {

    DescriptorSetAllocator::DescriptorSetAllocator() {}

    void DescriptorSetAllocator::init(vk::Device device) {
        this->device = device;
    }

    void DescriptorSetAllocator::destroy() {
        for (auto& pool : descriptor_pools)
            device.destroyDescriptorPool(pool);
        descriptor_pools.clear();
    }

    vk::DescriptorPool DescriptorSetAllocator::new_pool(const std::vector<vk::DescriptorPoolSize>& pool_sizes) {
        vk::DeviceSize total_pool_size = 0;
        for (auto& pool_size : pool_sizes) total_pool_size += pool_size.descriptorCount;
        vk::DescriptorPoolCreateInfo desc_pool_create_info({}, total_pool_size, pool_sizes);

        auto[result, descriptor_pool] = device.createDescriptorPool(desc_pool_create_info);
        CHECK_VK_RESULT(result, "Failed to create descriptor pool");

        descriptor_pools.push_back(descriptor_pool);
        return descriptor_pool;
    }


    DescriptorSetBuilder::DescriptorSetBuilder(DescriptorSetAllocator* descriptor_set_allocator, vk::Device device, uint multiplicity) : descriptor_set_allocator(descriptor_set_allocator), device(device), multiplicity(multiplicity) {}

    DescriptorSetBuilder& DescriptorSetBuilder::add_binding(vk::DescriptorSetLayoutBinding binding) {
        bindings.push_back(binding);
        pool_sizes.push_back({binding.descriptorType, multiplicity * binding.descriptorCount});
        return *this;
    }

    std::vector<vk::DescriptorSet> DescriptorSetBuilder::build() {
        vk::DescriptorPool descriptor_pool = descriptor_set_allocator->new_pool(pool_sizes);

        vk::DescriptorSetLayoutCreateInfo ds_layout_create_info({}, bindings);
        auto[cdsl_res, descriptor_set_layout] = device.createDescriptorSetLayout(ds_layout_create_info);
        CHECK_VK_RESULT(cdsl_res, "Failed to create descriptor set layout");
        layout = descriptor_set_layout;

        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts(multiplicity, descriptor_set_layout);
        vk::DescriptorSetAllocateInfo ds_allocate_info(descriptor_pool, descriptor_set_layouts);

        auto[ads_res, descriptor_sets] = device.allocateDescriptorSets(ds_allocate_info);
        CHECK_VK_RESULT(ads_res, "Failed to allocate descriptor sets");

        return descriptor_sets;
    }

}