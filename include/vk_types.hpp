#ifndef VK_TYPES_HPP
#define VK_TYPES_HPP

#define VULKAN_HPP_NO_EXCEPTIONS
#include <vulkan/vulkan.hpp>

#include <iostream>

#define CHECK_VK_RESULT(vk_result, msg) do { \
        if ((vk_result) != vk::Result::eSuccess) { \
            std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
        } \
    } while (0)

// #define CHECK_VK_RESULT_RF(vk_result, msg) do { \
//         if ((vk_result) != vk::Result::eSuccess) { \
//             std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
//             return false; \
//         } \
//     } while (0)

// #define CHECK_VK_RESULT_RN(vk_result, msg) do { \
//         if ((vk_result) != vk::Result::eSuccess) { \
//             std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
//             return nullptr; \
//         } \
//     } while (0)

#define CHECK_VK_RESULT_R(vk_result, return_if_err, msg) do { \
        if ((vk_result) != vk::Result::eSuccess) { \
            std::cerr << (msg) << ": " << int(vk_result) << std::endl; \
            return (return_if_err); \
        } \
    } while (0)

#endif