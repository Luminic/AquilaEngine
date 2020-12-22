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

        // Make sure everything has finished
        if (device)
            CHECK_VK_RESULT(device.waitIdle(), "Failed to wait for device to finish all tasks for cleanup");

        if (render_fence) device.destroyFence(render_fence);
        render_fence = nullptr;

        if (render_semaphore) device.destroySemaphore(render_semaphore);
        render_semaphore = nullptr;
        
        if (present_semaphore) device.destroySemaphore(present_semaphore);
        present_semaphore = nullptr;

        if (main_command_buffer) device.freeCommandBuffers(command_pool, main_command_buffer);
        main_command_buffer = nullptr;

        if (command_pool) device.destroyCommandPool(command_pool);
        command_pool = nullptr;

        for (auto& framebuffer : framebuffers) { device.destroyFramebuffer(framebuffer); }
        framebuffers.clear();

        for (auto& image_view : swap_chain_image_views) { device.destroyImageView(image_view); }
        swap_chain_image_views.clear();
        swap_chain_images.clear();

        if (render_pass) device.destroyRenderPass(render_pass);
        render_pass = nullptr;

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
    // Get extensions

    unsigned int sdl_extension_count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, nullptr)) return false;
    std::vector<const char*> sdl_extensions(sdl_extension_count);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &sdl_extension_count, sdl_extensions.data())) return false;

    for (auto extension : sdl_extensions) {
        extensions[std::string(extension)] = true;
    }

    device_extensions[VK_KHR_SWAPCHAIN_EXTENSION_NAME] = true;

    // Begin to actually initalize vulkan

    vk_init::VulkanInitializer vulkan_initializer(instance, chosen_gpu, device);

    if (!vulkan_initializer.create_instance("Vulkan Playground", 1, "None", 1, extensions)) return false;

    VkSurfaceKHR sdl_surface;
    SDL_Vulkan_CreateSurface(window, instance, &sdl_surface);
    surface = static_cast<vk::SurfaceKHR>(sdl_surface);

    if (!vulkan_initializer.choose_gpu(device_extensions, surface)) return false;
    gpu_properties = vulkan_initializer.get_gpu_properties();

    if (!vulkan_initializer.create_device(device_extensions)) return false;
    graphics_queue = device.getQueue(gpu_properties.graphics_present_queue_family, 0);

    if (!init_swapchain()) return false;
    if (!init_default_renderpass()) return false;
    if (!init_framebuffers()) return false;
    if (!init_commands()) return false;
    if (!init_sync_structures()) return false;

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

bool VulkanEngine::init_default_renderpass() {
    std::array<vk::AttachmentDescription, 1> attachment_descriptions{
        vk::AttachmentDescription( // Color Attachment
            {}, // flags
            surface_format.format,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear, // Load op
            vk::AttachmentStoreOp::eStore, // Store op
            vk::AttachmentLoadOp::eDontCare, // Stencil load op
            vk::AttachmentStoreOp::eDontCare, // Stencil store op
            vk::ImageLayout::eUndefined, // Initial layout
            vk::ImageLayout::ePresentSrcKHR // Final layout
        )
    };

    std::array<vk::AttachmentReference, 1> attachment_refs{
        {{0, vk::ImageLayout::eColorAttachmentOptimal}}
    };
    std::array<vk::SubpassDescription, 1> subpass_descriptions{
        vk::SubpassDescription(
            {}, // flags
            vk::PipelineBindPoint::eGraphics,
            {}, // input attachments count
            attachment_refs,
            {}, // resolve attachments
            {}, // depth stencil attachment
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
    CHECK_VK_RESULT_RF(crp_result, "Failed to create render pass");
    
    return true;
}

bool VulkanEngine::init_framebuffers() {
    vk::Result gsci_result;
    std::tie(gsci_result, swap_chain_images) = device.getSwapchainImagesKHR(swap_chain);
    CHECK_VK_RESULT_RF(gsci_result, "Failed to retrieve swap chain images");
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
        CHECK_VK_RESULT_RF(civ_result, "Failed to create image view");
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
    for (uint32_t i=0; i<image_count; ++i) {
        fb_create_info.attachmentCount = 1;
        fb_create_info.pAttachments = &swap_chain_image_views[i];
        vk::Result cf_result;
        std::tie(cf_result, framebuffers[i]) = device.createFramebuffer(fb_create_info);
        CHECK_VK_RESULT_RF(cf_result, "Failed to create framebuffer");
    }

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

bool VulkanEngine::init_sync_structures() {
    vk::FenceCreateInfo fence_create_info(vk::FenceCreateFlagBits::eSignaled);
    vk::Result cf_result;
    std::tie(cf_result, render_fence) = device.createFence(fence_create_info);
    CHECK_VK_RESULT_RF(cf_result, "Failed to create render fence");

    vk::SemaphoreCreateInfo semaphore_create_info{};
    vk::Result cs_result;
    std::tie(cs_result, render_semaphore) = device.createSemaphore(semaphore_create_info);
    CHECK_VK_RESULT_RF(cs_result, "Failed to create render semaphore");
    std::tie(cs_result, present_semaphore) = device.createSemaphore(semaphore_create_info);
    CHECK_VK_RESULT_RF(cs_result, "Failed to create present semaphore");

    return true;
}


void VulkanEngine::draw() {
    uint64_t timeout{ 1'000'000'000 };

    // Wait until the GPU has finished rendering the last frame
    CHECK_VK_RESULT( device.waitForFences(1, &render_fence, VK_TRUE, timeout), "Failed to wait for render fence");
	CHECK_VK_RESULT( device.resetFences(1, &render_fence), "Failed to reset render fence");

    // Get next swap chain image
    auto [ani_result, sw_ch_image_index] = device.acquireNextImageKHR(swap_chain, timeout, present_semaphore, {});
    CHECK_VK_RESULT(ani_result, "Failed to aquire next swap chain image");

    // Reset the command buffer
    CHECK_VK_RESULT(main_command_buffer.reset(), "Failed to reset main cmd buffer");

    // Begin command buffer
    vk::CommandBufferBeginInfo cmd_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    CHECK_VK_RESULT(main_command_buffer.begin(cmd_begin_info), "Failed to begin cmd buffer");

    uint64_t period = 2048;
    float flash = (frame_number%period) / float(period);
    std::array<vk::ClearValue, 1> clear_values{
        {vk::ClearColorValue{std::array<float,4>{0.0f,0.0f,flash,0.0f}}}
    };

    vk::RenderPassBeginInfo render_pass_begin_info(
        render_pass,
        framebuffers[sw_ch_image_index],
        vk::Rect2D({0,0}, window_extent),
        clear_values
    );

    main_command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);



    main_command_buffer.endRenderPass();

    // End command buffer
    CHECK_VK_RESULT(main_command_buffer.end(), "Failed to end command buffer");

    vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submit_info(1, &present_semaphore, &wait_stage, 1, &main_command_buffer, 1, &render_semaphore);
    CHECK_VK_RESULT(graphics_queue.submit(1, &submit_info, render_fence), "Failed to submit graphics_queue");

    vk::PresentInfoKHR present_info(1, &render_semaphore, 1, &swap_chain, &sw_ch_image_index);
    CHECK_VK_RESULT(graphics_queue.presentKHR(present_info), "Failed to present graphics queue");

    ++frame_number;
}