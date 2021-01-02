#ifndef SCENE_AQUILA_TEXTURE_HPP
#define SCENE_AQUILA_TEXTURE_HPP

#include <string>

#include "util/vk_types.hpp"

namespace aq {

    class Texture {
    public:
        Texture();

        bool upload_from_file(const char* path, vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        bool upload_from_data(void* data, int width, int height, vma::Allocator* allocator, const vk_util::UploadContext& upload_context);
        void destroy(); // Only works if the object was allocated/uploaded with a member function

        bool is_uploaded() { return bool(image.image); };
        vk::DescriptorImageInfo get_image_info(vk::Sampler sampler);

        AllocatedImage image;
        vk::ImageView image_view;
        vk::Extent2D size;
        std::string path;
    
    private:
        vk::Device device;
    };

}

#endif