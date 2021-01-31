#include "scene/aq_texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace aq {

    Texture::Texture() {}

    Texture::~Texture() { destroy(); }

    void Texture::upload_later(std::string path) {
        this->path = path;
    }

    void Texture::upload_later(std::string embedded_path, unsigned char* data, int width, int height, std::string format_hint) {
        this->path = embedded_path;
        size.width = width;
        size.height = height;

        size_t data_size;
        if (height == 0) data_size = (size_t) width;
        else data_size = (size_t) (width * height);

        this->data.insert(std::end(this->data), &data[0], &data[data_size]);
        std::cout << this->data.size() << '\n';
        this->format_hint = format_hint;
    }

    bool Texture::upload(vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        if (path.empty())
            return false;

        if (data.empty()) {
            return upload_from_file(path, allocator, upload_context);
        } else { // Embedded Texture -- load from data
            if (size.height == 0) { // Compressed (eg. png, jpg, etc.)
                int texture_width, texture_height, nr_channels;
                stbi_uc* pixels = stbi_load_from_memory(data.data(), size.width, &texture_width, &texture_height, &nr_channels, STBI_rgb_alpha);
                if (!pixels) {
                    std::cerr << "Failed to load image " << path << " from memory: " << stbi_failure_reason() << '\n';
                    return false;
                }
                size.width = texture_width;
                size.height = texture_height;
                bool result = upload_from_data(pixels, size.width, size.height, allocator, upload_context);
                stbi_image_free(pixels);
                data.clear(); // the data is no longer needed
                return result;

            } else {
                // Hopefully RGBA8888 format
                if (!format_hint.empty() && format_hint != "rgba8888")
                    std::cerr << "Unsupported format: " << format_hint << ". Loading as rgba8888. Expect malformed textures.\n";
                return upload_from_data(data.data(), size.width, size.height, allocator, upload_context);
            }
        }
    }

    bool Texture::upload_from_file(std::string path, vma::Allocator* allocator, const vk_util::UploadContext& upload_context) {
        this->path = path;
        int texture_width, texture_height, nr_channels;
        stbi_uc* pixels = stbi_load(path.c_str(), &texture_width, &texture_height, &nr_channels, STBI_rgb_alpha);
        if (!pixels) {
            std::cerr << "Failed to load texture file " << path << ": " << stbi_failure_reason() << '\n';
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

        if (!image.upload(data, width, height, allocator, upload_context)) return false;

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