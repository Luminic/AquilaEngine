# AquilaEngine
A place for me to play around in Vulkan

This is a 3D toy engine / rasterizer created using Vulkan and C++. Currently, it is capable of rendering 3D models in a node hierarchy, uploading textures and material data to the GPU, and basic Phong diffuse lighting.

Currently, PBR Lighting is being implemented.

## Timeline:

3/26/2021 - Point Lights! (195 hours)

1/4/2021 - Development Paused: Busy with school

1/3/2021 - Material System! (123 hours of development)

1/1/2021 - Textures! (100 hours of development)

12/29/2020 - Mesh Loading and Node Hierarchies! (76 hours of development)

12/22/2020 - Hello Triangle! (31 hours of development)

12/16/2020 - Project Started!

## Screenshots:

![point lights](https://github.com/Luminic/AquilaEngine/blob/master/screenshots/point_lights_2021-03-28.png)

![textures](https://github.com/Luminic/AquilaEngine/blob/master/screenshots/textures_2021-01-01_2.png)

![monkey helix](https://github.com/Luminic/AquilaEngine/blob/master/screenshots/model_loading_2020-12-29_2.png)

![materials](https://github.com/Luminic/AquilaEngine/blob/master/screenshots/materials_2021-01-02.png)


## Credits:
### Libraries:
Vulkan - Rendering / GPU control (https://www.khronos.org/vulkan/)

Vulkan Memory Allocator - Allocating memory for the GPU (https://gpuopen.com/vulkan-memory-allocator/)

SDL2 - Windowing (https://www.libsdl.org/)

Dear ImGui - User Interface (https://github.com/ocornut/imgui)

Assimp - Model loading (https://www.assimp.org/)

STB Image - Image Loading (https://github.com/nothings/stb)

### Tutorials
Below is a list of the notable tutorials I followed when creating this engine:

https://vkguide.dev/

https://vulkan-tutorial.com/

https://learnopengl.com/

