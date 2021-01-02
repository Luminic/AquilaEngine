#include "scene/aq_texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace aq {

    Texture::Texture() {}

    bool Texture::upload_from_file(const char* path, vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        this->path = std::string(path);

        int texture_width, texture_height, nr_channels;
        stbi_uc* pixels = stbi_load(path, &texture_width, &texture_height, &nr_channels, STBI_rgb_alpha);
        if (!pixels) {
            std::cerr << "Failed to load texture file " << path << '\n';
            return false;
        }
        bool result = upload_from_data(pixels, texture_width, texture_height, allocator, upload_context);

        stbi_image_free(pixels);

        return result;
    }

    bool Texture::upload_from_data(void* data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        device = upload_context.device;
        size.width = width;
        size.height = height;

        if (!image.upload_from_data(data, width, height, allocator, upload_context)) return false;

        vk::ImageViewCreateInfo image_view_create_info(
            {},
            image.image,
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Unorm,
            {vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity},
            {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        );

        vk::Result civ_res;
        std::tie(civ_res, image_view) = device.createImageView(image_view_create_info);
        CHECK_VK_RESULT_R(civ_res, false, "Failed to create image view");

        return true;
    }

    void Texture::destroy() {
        image.destroy();
        if (device) {
            device.destroyImageView(image_view);
            image_view = nullptr;
            device = nullptr;
            size = vk::Extent2D{};
        }
    }

    vk::DescriptorImageInfo Texture::get_image_info(vk::Sampler sampler) {
        return vk::DescriptorImageInfo(
            sampler, image_view, vk::ImageLayout::eShaderReadOnlyOptimal
        );
    }

}