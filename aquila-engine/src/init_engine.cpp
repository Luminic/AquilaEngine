#include "init_engine.hpp"

#include <iostream>

#include <SDL2/SDL_vulkan.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/gtc/matrix_transform.hpp>

namespace aq {

    InitializationEngine::InitializationEngine() {}

    InitializationEngine::~InitializationEngine() {
        cleanup(); // Just in case
    }

    InitializationEngine::InitializationState InitializationEngine::init() {
        SDL_Init(SDL_INIT_VIDEO);
        SDL_WindowFlags window_flags = SDL_WindowFlags(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        window = SDL_CreateWindow(
            "Aquila Engine",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            window_extent.width,
            window_extent.height,
            window_flags
        );

        if (!window) {
            std::cerr << "SDL window failed to initialze: " << SDL_GetError() << std::endl;
        }

        if (!init_vulkan_resources()) initialization_state = InitializationState::FailedVulkanObjectsInitialization;
        if (!init_render_resources()) initialization_state = InitializationState::FailedInitialization;
        if (!init_swapchain_resources()) initialization_state = InitializationState::FailedSwapChainInitialization;

        return initialization_state;
    }

    bool InitializationEngine::wait_idle() {
        if (device) {
            CHECK_VK_RESULT_R(device.waitIdle(), false, "Failed to wait for device to idle");
            return true;
        }
        return false;
    }

    void InitializationEngine::cleanup() {
        wait_idle();

        cleanup_render_resources();
        cleanup_swapchain_resources();
        cleanup_vulkan_resources();

        if (window) SDL_DestroyWindow(window);
        window = nullptr;
    }

    InitializationEngine::InitializationState InitializationEngine::get_initialization_state() {
        return initialization_state;
    }

    bool InitializationEngine::init_vulkan_resources() {
        // Get extensions

        unsigned int sdl_extension_count = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr)) {
            std::cerr << "Failed to query SDL Vulkan extensions" << std::endl;
            return false; }
        std::vector<const char*> sdl_extensions(sdl_extension_count);
        if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions.data())) {
            std::cerr << "Failed to query SDL Vulkan extensions" << std::endl;
            return false; }

        for (auto extension : sdl_extensions) {
            extensions[std::string(extension)] = true;
        }

        device_extensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;

        // Begin to actually initalize vulkan

        vk_init::VulkanInitializer vulkan_initializer(instance, chosen_gpu, device);

        if (!vulkan_initializer.create_instance("Vulkan Playground", 1, "aquila-engine", 1, extensions)) return false;

        VkSurfaceKHR sdl_surface;
        SDL_Vulkan_CreateSurface(window, instance, &sdl_surface);
        surface = static_cast<vk::SurfaceKHR>(sdl_surface);
        deletion_queue.push_function([this]() { instance.destroy(surface); });

        if (!vulkan_initializer.choose_gpu(device_extensions, surface)) return false;
        gpu_support = vulkan_initializer.get_gpu_support();
        gpu_properties = chosen_gpu.getProperties();

        if (!vulkan_initializer.create_device(device_extensions)) return false;
        graphics_queue = device.getQueue(gpu_support.graphics_present_queue_family, 0);

        vma::AllocatorCreateInfo allocator_create_info({}, chosen_gpu, device);
        allocator_create_info.instance = instance;
        allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
        vk::Result ca_result;
        std::tie(ca_result, allocator) = vma::createAllocator(allocator_create_info);
        CHECK_VK_RESULT_R(ca_result, false, "Failed to create vma allocator");
        deletion_queue.push_function([this]() { allocator.destroy(); });

        if (!init_command_pool()) return false;
        if (!choose_surface_format()) return false;
        if (!init_default_renderpass()) return false;

        initialization_state = InitializationState::VulkanObjectsInitialized;

        return true;
    }

    void InitializationEngine::cleanup_vulkan_resources() {
        if (int(initialization_state) > int(InitializationState::VulkanObjectsInitialized)) {
            std::cerr << "Vulkan resources can only be cleaned up if `initialization_state` is `InitializationState::VulkanObjectsInitialized` or before.\n";
            std::cerr << "Current initialization state: " << int(initialization_state) << std::endl;
            return;
        }

        deletion_queue.flush();

        if (device) device.destroy();
        device = nullptr;

        if (instance) instance.destroy();
        instance = nullptr;

        initialization_state = InitializationState::Uninitialized;
    }

    bool InitializationEngine::init_swapchain_resources() {
        if (initialization_state != InitializationState::VulkanObjectsInitialized) {
            std::cerr << "Initialization state must be " << int(InitializationState::VulkanObjectsInitialized) << " for `init_swapchain_resources`" << std::endl;
            return false;
        }

        if (!init_command_buffers()) return false;
        if (!init_swapchain()) return false;
        if (!init_framebuffers()) return false;
        if (!init_sync_structures()) return false;

        initialization_state = InitializationState::Initialized;

        return true;
    }

    void InitializationEngine::cleanup_swapchain_resources() {
        // Make sure everything has finished
        wait_idle();
        
        swap_chain_deletion_queue.flush();

        initialization_state = InitializationState::VulkanObjectsInitialized;
    }

    bool InitializationEngine::init_render_resources() {return true;}

    void InitializationEngine::cleanup_render_resources() {}

    bool InitializationEngine::resize_window() {
        cleanup_swapchain_resources();
        return init_swapchain_resources();
    }

    bool InitializationEngine::init_command_pool() {
        vk::CommandPoolCreateInfo command_pool_create_info(
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            graphics_queue_family
        );

        for (uint i=0; i<FRAME_OVERLAP; ++i) {
            vk::Result ccp_result;
            std::tie(ccp_result, frame_objects[i].command_pool) = device.createCommandPool(command_pool_create_info);
            CHECK_VK_RESULT_R(ccp_result, false, "Failed to create command pool");

            deletion_queue.push_function([this, i](){
                if (frame_objects[i].command_pool) device.destroyCommandPool(frame_objects[i].command_pool);
                frame_objects[i].command_pool = nullptr;
            });
        }


        return true;
    }

    bool InitializationEngine::choose_surface_format() {
        vk_init::SwapChainSupportDetails& sw_ch_support = gpu_support.sw_ch_support;

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

        return true;
    }

    bool InitializationEngine::init_default_renderpass() {
        depth_format = vk::Format::eD32Sfloat;
        
        std::array<vk::AttachmentDescription, 2> attachment_descriptions{
            vk::AttachmentDescription( // Color Attachment
                {}, // flags
                surface_format.format,
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, // Load op
                vk::AttachmentStoreOp::eStore, // Store op
                vk::AttachmentLoadOp::eClear, // Stencil load op
                vk::AttachmentStoreOp::eDontCare, // Stencil store op
                vk::ImageLayout::eUndefined, // Initial layout
                vk::ImageLayout::ePresentSrcKHR // Final layout
            ),
            vk::AttachmentDescription( // Depth Attachment
                {}, // flags
                depth_format,
                vk::SampleCountFlagBits::e1,
                vk::AttachmentLoadOp::eClear, // Load op
                vk::AttachmentStoreOp::eStore, // Store op
                vk::AttachmentLoadOp::eClear, // Stencil load op
                vk::AttachmentStoreOp::eDontCare, // Stencil store op
                vk::ImageLayout::eUndefined, // Initial layout
                vk::ImageLayout::eDepthStencilAttachmentOptimal // Final layout
            )
        };

        std::array<vk::AttachmentReference, 1> color_attachment_refs{{
            {0, vk::ImageLayout::eColorAttachmentOptimal}
        }};

        vk::AttachmentReference depth_attachment_ref(
            1, vk::ImageLayout::eDepthStencilAttachmentOptimal
        );

        std::array<vk::SubpassDescription, 1> subpass_descriptions{
            vk::SubpassDescription(
                {}, // flags
                vk::PipelineBindPoint::eGraphics,
                {}, // input attachments count
                color_attachment_refs,
                {}, // resolve attachments
                &depth_attachment_ref,
                {} // preserve attachments
            )
        };

        vk::RenderPassCreateInfo render_pass_info(
            {}, // flags
            attachment_descriptions,
            subpass_descriptions,
            {} // Subpass dependencies
        );

        vk::Result crp_result;
        std::tie(crp_result, render_pass) = device.createRenderPass(render_pass_info);
        CHECK_VK_RESULT_R(crp_result, false, "Failed to create render pass");

        deletion_queue.push_function([this]() { device.destroyRenderPass(render_pass); });

        return true;
    }

    bool InitializationEngine::init_command_buffers() {
        vk::CommandBufferAllocateInfo cmd_buff_alloc_info(nullptr, vk::CommandBufferLevel::ePrimary, 1);

        for (uint i=0; i<FRAME_OVERLAP; ++i) {
            cmd_buff_alloc_info.commandPool = frame_objects[i].command_pool;

            auto[acb_result, cmd_buffs] = device.allocateCommandBuffers(cmd_buff_alloc_info);
            CHECK_VK_RESULT_R(acb_result, false, "Failed to allocate command buffer(s)");
            swap_chain_deletion_queue.push_function([this, i]() { device.freeCommandBuffers(frame_objects[i].command_pool, frame_objects[i].main_command_buffer); });

            frame_objects[i].main_command_buffer = cmd_buffs[0];
        }

        return true;
    }

    bool InitializationEngine::init_swapchain() {
        vk_init::SwapChainSupportDetails& sw_ch_support = gpu_support.sw_ch_support;

        // Make sure the swap chain support details are up to date
        sw_ch_support.update(chosen_gpu, surface);

        // Make sure surface format is still supported
        bool surface_format_supported = false;
        for (auto& available_surface_format : sw_ch_support.formats) {
            if (surface_format == available_surface_format) {
                surface_format_supported = true;
                break;
            }
        }
        if (!surface_format_supported) {
            std::cerr << "Surface format no longer supported; aborting." << std::endl;
            return false;
        }

        // Get present mode
        present_mode = vk::PresentModeKHR::eFifo; // Always supported
        for (auto& available_present_mode : sw_ch_support.present_modes) {
            if (available_present_mode == vk::PresentModeKHR::eMailbox) {
                present_mode = available_present_mode;
            }
        }

        // Get extent
        if (sw_ch_support.capabilities.currentExtent.width == UINT32_MAX) {
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

        // Create depth buffer
        vk::Extent3D depth_image_extent(window_extent.width, window_extent.height, 1);

        vk::ImageCreateInfo depth_img_info(
            {}, // Flags
            vk::ImageType::e2D,
            depth_format,
            depth_image_extent,
            1, // mip levels
            1, // array layers
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment
        );

        vma::AllocationCreateInfo depth_img_alloc_info(
            {}, // Allocation create flags
            vma::MemoryUsage::eGpuOnly,
            vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eDeviceLocal)
        );

        auto [cdi_result, img_alloc] = allocator.createImage(depth_img_info, depth_img_alloc_info);
        depth_image.set(img_alloc);
        CHECK_VK_RESULT_R(cdi_result, false, "Failed to create depth image");
        swap_chain_deletion_queue.push_function([this]() { allocator.destroyImage(depth_image.image, depth_image.allocation); });

        vk::ImageViewCreateInfo depth_img_view_info(
            {}, // flags
            depth_image.image,
            vk::ImageViewType::e2D,
            depth_format,
            {vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
            { // subresource range
                vk::ImageAspectFlagBits::eDepth,
                0, // base mip level
                1, // level count
                0, // base array level
                1 // layer count
            }
        );

        vk::Result cdiv_result;
        std::tie(cdiv_result, depth_image_view) = device.createImageView(depth_img_view_info);
        CHECK_VK_RESULT_R(cdiv_result, false, "Failed to create depth image view");
        swap_chain_deletion_queue.push_function([this]() { device.destroyImageView(depth_image_view); });

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
        CHECK_VK_RESULT_R(csc_result, false, "Failed to create swap chain");
        swap_chain_deletion_queue.push_function([this]() { device.destroySwapchainKHR(swap_chain); });

        return true;
    }

    bool InitializationEngine::init_framebuffers() {
        swap_chain_deletion_queue.push_function([this]() {
            for (auto& framebuffer : framebuffers) { device.destroyFramebuffer(framebuffer); }
            framebuffers.clear();

            for (auto& image_view : swap_chain_image_views) { device.destroyImageView(image_view); }
            swap_chain_image_views.clear();
            swap_chain_images.clear(); // Swap chain images are created by the swapchain so I don't need to delete them myself
        });

        vk::Result gsci_result;
        std::tie(gsci_result, swap_chain_images) = device.getSwapchainImagesKHR(swap_chain);
        CHECK_VK_RESULT_R(gsci_result, false, "Failed to retrieve swap chain images");
        image_count = swap_chain_images.size();

        swap_chain_image_views.resize(image_count);
        vk::ImageViewCreateInfo image_view_create_info(
            {}, // flags
            {}, // image (set later)
            vk::ImageViewType::e2D,
            surface_format.format,
            vk::ComponentMapping(
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity,
                vk::ComponentSwizzle::eIdentity
            ),
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eColor,
                0, // baseMipLevel
                1, // levelCount
                0, // baseArrayLayer
                1 // layerCount
            )
        );
        for (uint32_t i=0; i<image_count; ++i) {
            image_view_create_info.image = swap_chain_images[i];
            vk::Result civ_result;
            std::tie(civ_result, swap_chain_image_views[i]) = device.createImageView(image_view_create_info);
            CHECK_VK_RESULT_R(civ_result, false, "Failed to create image view");
        }

        framebuffers.resize(image_count);
        vk::FramebufferCreateInfo fb_create_info(
            {}, // flags
            render_pass,
            {}, // image view(s) (set later)
            window_extent.width,
            window_extent.height,
            1 // layers
        );

        std::array<vk::ImageView, 2> attachments({nullptr, depth_image_view});
        for (uint32_t i=0; i<image_count; ++i) {
            attachments[0] = swap_chain_image_views[i];
            fb_create_info.setAttachments(attachments);

            vk::Result cf_result;
            std::tie(cf_result, framebuffers[i]) = device.createFramebuffer(fb_create_info);
            CHECK_VK_RESULT_R(cf_result, false, "Failed to create framebuffer");
        }

        return true;
    }

    bool InitializationEngine::init_sync_structures() {
        for (uint i=0; i<FRAME_OVERLAP; ++i) {

            vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
            vk::Result cf_result;
            std::tie(cf_result, frame_objects[i].render_fence) = device.createFence(fence_create_info);
            CHECK_VK_RESULT_R(cf_result, false, "Failed to create render fence");
            swap_chain_deletion_queue.push_function([this, i]() { device.destroyFence(frame_objects[i].render_fence); });


            vk::SemaphoreCreateInfo semaphore_create_info{};
            vk::Result cs_result;

            std::tie(cs_result, frame_objects[i].render_semaphore) = device.createSemaphore(semaphore_create_info);
            CHECK_VK_RESULT_R(cs_result, false, "Failed to create render semaphore");
            swap_chain_deletion_queue.push_function([this, i]() { device.destroySemaphore(frame_objects[i].render_semaphore); });

            std::tie(cs_result, frame_objects[i].present_semaphore) = device.createSemaphore(semaphore_create_info);
            CHECK_VK_RESULT_R(cs_result, false, "Failed to create present semaphore");
            swap_chain_deletion_queue.push_function([this, i]() { device.destroySemaphore(frame_objects[i].present_semaphore); });
        }

        return true;
    }

}