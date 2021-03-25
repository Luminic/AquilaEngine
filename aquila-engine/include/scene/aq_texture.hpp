#ifndef SCENE_AQUILA_TEXTURE_HPP
#define SCENE_AQUILA_TEXTURE_HPP

#include <string>
#include <vector>

#include "util/vk_types.hpp"

namespace aq {

    class Texture {
    public:
        Texture();
        ~Texture();

        void upload_later(std::string path);
        // Embedded path should be unique to the texture. It is used for finding texture hashes
        // `data` will be copied into a private buffer
        // If `height` is 0, the size of data should be `width` and it will be read as a "compressed format" (eg. png, jpg, etc.)
        void upload_later(std::string embedded_path, unsigned char* data, int width, int height, std::string format_hint={});
        // Uploads texture with data from `upload_later`
        bool upload(vma::Allocator* allocator, const vk_util::UploadContext& upload_context);

        // Use texture path
        bool upload_from_file(std::string path, vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        bool upload_from_data(void* data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        void clear_cpu_data();
        void destroy(); // Only works if the object was allocated/uploaded with a member function

        bool is_uploaded() { return bool(image.image); };
        const std::string& get_path() { return path; };
        vk::DescriptorImageInfo get_image_info(vk::Sampler sampler=nullptr);

        AllocatedImage image;
        vk::ImageView image_view;
        vk::Extent2D size;
    
    private:
        std::string path;

        // Used for `upload_later` with embedded data
        std::vector<unsigned char> data;
        std::string format_hint;

        vk::Device device;
    };

}

#endif