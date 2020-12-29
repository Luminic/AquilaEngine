#ifndef VK_TYPES_HPP
#define VK_TYPES_HPP

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_ASSERT_ON_RESULT
#include <vulkan/vulkan.hpp>

#include <vk_mem_alloc.hpp>

#include <iostream>

#define CHECK_VK_RESULT(vk_result, msg) do { \
        if ((vk_result) != vk::Result::eSuccess) { \
            std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
        } \
    } while (0)

#define CHECK_VK_RESULT_R(vk_result, return_if_err, msg) do { \
        if ((vk_result) != vk::Result::eSuccess) { \
            std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
            return (return_if_err); \
        } \
    } while (0)

namespace aq {

    struct AllocatedBuffer {
        vk::Buffer buffer;
        vma::Allocation allocation;

        void operator=(const std::pair<vk::Buffer, vma::Allocation>& lhs) {
            buffer = lhs.first;
            allocation = lhs.second;
        }
    };

    struct AllocatedImage {
        vk::Image image;
        vma::Allocation allocation;

        void operator=(const std::pair<vk::Image, vma::Allocation>& lhs) {
            image = lhs.first;
            allocation = lhs.second;
        }
    };

}

#endif
